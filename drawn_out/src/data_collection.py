import http.client
import os
import time
import urllib3
import logging
import json
from time import sleep
from typing import Optional, Tuple
from datetime import datetime

import bs4
import libsql_experimental as libsql
from selenium import webdriver
from selenium.common.exceptions import NoSuchElementException
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.support.wait import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC

DB_URI = os.getenv("DB_URI")
DB_TOKEN = os.getenv("DB_TOKEN")
SELENIUM_HOST = os.getenv("SELENIUM_HOST")
SELENIUM_PORT = os.getenv("SELENIUM_PORT")

API_HOST = 'www.chess.com'
TT_ROUNDS = 11

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[ logging.StreamHandler() ]
)
logger = logging.getLogger("tt_scraper")

# Add metrics dict to track timings
metrics = {
    "selenium_request_times": [],
    "db_call_intervals": [],
    "selenium_connect_times": [],
    "last_db_call_time": None
}


def log_metrics():
    """Log the current metrics to the console"""
    if metrics["selenium_request_times"]:
        avg_selenium_time = sum(metrics["selenium_request_times"]) / len(metrics["selenium_request_times"])
        max_selenium_time = max(metrics["selenium_request_times"])
        logger.info(f"Selenium metrics - Avg request time: {avg_selenium_time:.2f}s, Max: {max_selenium_time:.2f}s, Count: {len(metrics['selenium_request_times'])}")
    
    if metrics["db_call_intervals"]:
        avg_db_interval = sum(metrics["db_call_intervals"]) / len(metrics["db_call_intervals"])
        max_db_interval = max(metrics["db_call_intervals"])
        logger.info(f"DB metrics - Avg interval between calls: {avg_db_interval:.2f}s, Max: {max_db_interval:.2f}s, Count: {len(metrics['db_call_intervals'])}")
    
    if metrics["selenium_connect_times"]:
        avg_connect_time = sum(metrics["selenium_connect_times"]) / len(metrics["selenium_connect_times"])
        max_connect_time = max(metrics["selenium_connect_times"])
        logger.info(f"Selenium connection metrics - Avg connect time: {avg_connect_time:.2f}s, Max: {max_connect_time:.2f}s, Count: {len(metrics['selenium_connect_times'])}")


def connect_to_selenium() -> webdriver.remote.webdriver.WebDriver:
    """Connect to Selenium remote server with timing metrics"""
    logger.info(f"Connecting to Selenium at {SELENIUM_HOST}:{SELENIUM_PORT}")
    start_time = time.time()
    options = Options()
    driver = webdriver.Remote(
        command_executor=f"{SELENIUM_HOST}:{SELENIUM_PORT}",
        options=options
    )
    connect_time = time.time() - start_time
    metrics["selenium_connect_times"].append(connect_time)
    logger.info(f"Connected to Selenium in {connect_time:.2f}s")
    return driver


def main():
    logger.info('Starting data collection process')
    if DB_URI is None or DB_TOKEN is None or SELENIUM_HOST is None or SELENIUM_PORT is None :
        logger.error('Required environement variables are set in the environment')
        return

    db = libsql_connect()
    run_migrations(db)
    
    while True:
        tournament_id = fetch_current_job(db)
        if tournament_id is None:
            logger.info("No pending jobs found")
            return

        logger.info(f"Processing tournament ID: {tournament_id}")
        populate_tournament_rounds(tournament_id)
        populate_game_pgns(tournament_id)

        # Check for completion of a job and update
        check_and_set_completed(tournament_id)
        
    
    log_metrics()  # Log final metrics summary


def libsql_connect() -> libsql.Connection:
    """Wrapper for database connection that tracks metrics"""
    now = time.time()
    if metrics["last_db_call_time"] is not None:
        interval = now - metrics["last_db_call_time"]
        metrics["db_call_intervals"].append(interval)
        logger.debug(f"Time since last DB call: {interval:.2f}s")
    
    metrics["last_db_call_time"] = now
    return libsql.connect(DB_URI, auth_token=DB_TOKEN)


def run_migrations(db: libsql.Connection):
    logger.info("Running database migrations")
    with open('migrations/init.sql') as f:
        db.executescript(f.read())
        db.commit()
   
 
def fetch_current_job(db: libsql.Connection) -> Optional[str]:
    logger.info("Fetching current job from database")
    pending_jobs = db.execute(
        "SELECT tournament_id FROM jobs WHERE status = 'pending' LIMIT 1;"
    ).fetchone()
    if pending_jobs is not None:
        return pending_jobs[0]

    pending_jobs = db.execute('''
        UPDATE jobs
        SET status = 'pending'
        WHERE id = (
            SELECT id FROM jobs
            WHERE status = 'not_asked'
            LIMIT 1
        )
        RETURNING tournament_id;'''
    ).fetchone()
    db.commit()
    
    if pending_jobs is not None:
        return pending_jobs[0]
    
    return None


def check_and_set_completed(tournament_id: str):
    logger.info(f"Checking job {tournament_id} for completeness")
    db = libsql_connect()
    res = db.execute(
        "SELECT rounds_fetched FROM tournament WHERE id = ?;", 
        (tournament_id,)
    ).fetchone()
    if res is None:
        logger.error(f"Could not check job {tournament_id} for completeness")
        return
    rounds_fetched = res[0] 
    
    res = db.execute("""
        SELECT 
          COUNT(g.id) 
        FROM 
          `game` AS g 
          LEFT JOIN tournament AS t ON g.tournament_id = t.id 
        WHERE 
          t.id = ?
          AND NOT (
            g.status = "success" 
            OR (
              g.status = "error" 
              AND g.retries >= 3
            )
          );""",
        (tournament_id,)
    ).fetchone()
    if res is None:
        logger.error(f"Could not check job {tournament_id} for completeness")
        return
    unprocessed_games = res[0] 
    
    if rounds_fetched > 11 and unprocessed_games == 0:
        logger.info(f"Setting {tournament_id} as complete")
        db.execute("UPDATE jobs SET status = 'complete' WHERE tournament_id = ?;", (tournament_id,))
        db.commit()
    
    


def populate_tournament_rounds(tournament_id: str):
    logger.info(f"Populating tournament rounds for tournament {tournament_id}")
    while True:
        db = libsql_connect()
        res = db.execute(
            "SELECT rounds_fetched FROM tournament WHERE id = ?1;", 
            ( tournament_id, )
        ).fetchone()
        
        if res is None:
            logger.error(f"Tournament {tournament_id} not found in database")
            return
        rounds_fetched = res[0]
        
        if rounds_fetched > TT_ROUNDS:
            return

        current_round = rounds_fetched + 1

        logger.info(f"Fetching game IDs for tournament {tournament_id}, round {current_round}")
        round_ids = get_game_ids(tournament_id, current_round)
        logger.info(f"Found {len(round_ids)} games for round {current_round}")
        
        db = libsql_connect()
        db.executemany(
            """INSERT INTO game (id, tournament_id, round, pgn) VALUES (?, ?, ?, "") ON CONFLICT DO NOTHING;""",
	        list(map(lambda game_id: (game_id, tournament_id, current_round), round_ids))
	    )
        db.execute(
            "UPDATE tournament SET rounds_fetched = rounds_fetched + 1 WHERE id = ?", 
            ( tournament_id, )
        ).fetchone()
        db.commit()


def populate_game_pgns(tournament_id: str, local_max_retries: int = 3):
    """
    Populates game PGNs for a tournament with local retries for Selenium connection errors.
    
    Args:
        tournament_id: The ID of the tournament to populate PGNs for
        local_max_retries: Maximum number of local retries before updating database status
    """
    logger.info(f"Populating game PGNs for tournament {tournament_id}")
    driver = None
    
    # Local retry tracking dictionary
    local_retry_tracker = {}  # game_id -> retry_count
    
    try:
        driver = connect_to_selenium()
        
        while True:
            db = libsql_connect()
            res = db.execute("""
                SELECT COUNT(*) AS matching_count
                FROM game
                WHERE
                    tournament_id = ?
                    AND (status = 'not_asked' OR status = 'error')
                    AND retries < 3;""",
                (tournament_id,)
            ).fetchone()
            
            if res is None:
                logger.error("Error querying game count")
                return
                
            count = res[0]
            logger.info(f"Found {count} games to process for tournament {tournament_id}")
            if count == 0:
                break
                
            res = db.execute("""
                SELECT "id" 
                FROM game
                WHERE
                    tournament_id = ? 
                    AND (status = 'not_asked' OR status = 'error')
                    AND retries < 3
                LIMIT 50;""",
                (tournament_id,)
            ).fetchall()

            # Process batch of games with local retry mechanism
            for (game_id,) in res:
                success = False
                
                # Initialize retry count if not present
                if game_id not in local_retry_tracker:
                    local_retry_tracker[game_id] = 0
                
                while local_retry_tracker[game_id] < local_max_retries and not success:
                    try:
                        start_time = time.time()
                        logger.info(f"Fetching PGN for game {game_id} (local retry {local_retry_tracker[game_id]})")
                        
                        pgn = get_pgn(driver, game_id)
                        
                        # Record selenium request time
                        request_time = time.time() - start_time
                        metrics["selenium_request_times"].append(request_time)
                        logger.debug(f"Selenium request completed in {request_time:.2f}s for game {game_id}")
                        
                        if pgn is not None:
                            logger.info(f"Successfully fetched PGN for game {game_id}")
                            db = libsql_connect()
                            db.execute(
                                """UPDATE game SET status = 'success', pgn = ?2 WHERE id = ?1;""", 
                                (game_id, pgn)
                            )
                            db.commit()
                            success = True
                        else:
                            # Increment local retry count on failure
                            local_retry_tracker[game_id] += 1
                            logger.warning(f"Failed to fetch PGN for game {game_id} (local retry {local_retry_tracker[game_id]}/{local_max_retries})")
                            
                            # Wait before retry with exponential backoff
                            backoff_time = 2 ** local_retry_tracker[game_id]
                            logger.info(f"Waiting {backoff_time} seconds before retrying")
                            time.sleep(backoff_time)
                            
                            # Check if we need to reconnect to Selenium
                            if driver is None or not is_driver_active(driver):
                                logger.info("Reconnecting to Selenium")
                                if driver is not None:
                                    try:
                                        driver.quit()
                                    except:
                                        pass
                                driver = connect_to_selenium()
                    
                    except Exception as e:
                        logger.warning(f"Selenium error while fetching PGN for game {game_id}: {str(e)}")
                        local_retry_tracker[game_id] += 1
                        
                        # Wait before retry with exponential backoff
                        backoff_time = 2 ** local_retry_tracker[game_id]
                        logger.info(f"Waiting {backoff_time} seconds before retrying")
                        time.sleep(backoff_time)
                        
                        # Reconnect to Selenium on error
                        if driver is not None:
                            try:
                                driver.quit()
                            except:
                                pass
                        driver = connect_to_selenium()
                
                # Update DB status only after all local retries are exhausted
                if not success:
                    db = libsql_connect()
                    logger.warning(f"All local retries failed for game {game_id}, updating database status")
                    db.execute( 
                        """UPDATE game SET status = 'error', retries = retries + 1 WHERE id = ?;""",
                        (game_id,)
                    )
                    db.commit()
                
                # Reset local retry count after processing
                local_retry_tracker.pop(game_id, None)
    
    except Exception as e:
        logger.exception(f"Error in populate_game_pgns: \n{e}")
        
    finally:
        if driver is not None:
            try:
                driver.quit()
            except:
                pass
        
    logger.info(f"Batch of games processed, logging interim metrics")
    log_metrics()


def is_driver_active(driver):
    """Check if the Selenium driver is still active and responsive"""
    try:
        # Try to execute a simple command to check if the driver is responsive
        driver.title
        return True
    except:
        return False

def get_game_ids(tournament_id: str, tournament_round: int) -> [str]:
    game_ids = []
    
    current_page = 1
    conn = http.client.HTTPSConnection(API_HOST)
    conn.request(
        'GET', 
        get_tournament_round_uri(
            tournament_id, 
            tournament_round, 
            current_page
        )
    )
    resp = conn.getresponse()
    soup = bs4.BeautifulSoup(resp.read(), 'html.parser')
    game_ids.extend(get_game_ids_from_page(soup))

    page_count = 1
    try:
        page_count = int(get_total_pages(soup))
    except:
        logger.exception("Error determining total pages")
        pass
     
    if page_count <= 1:
        return game_ids
    
    for i in range(2, page_count + 1):
        conn.request(
            'GET', 
            get_tournament_round_uri(
                tournament_id, 
                tournament_round , 
                i
            )
        )
        resp = conn.getresponse()
        soup = bs4.BeautifulSoup(resp.read(), 'html.parser')
        game_ids.extend(get_game_ids_from_page(soup))

    return game_ids


def get_pgn(driver: webdriver.chrome.webdriver.WebDriver, game_id: str, page_delay=10) -> Optional[str] :
    try:
        driver.get(f'https://www.chess.com/game/live/{game_id}')
        WebDriverWait(driver, 120).until(
            EC.presence_of_element_located((
                By.CSS_SELECTOR, 
                '.board-modal-header-close[aria-label="Close"]'
            ))
        )
        
        wait = WebDriverWait(driver, 10)
        # Sleep to account for layout shifts
        sleep(2)
        wait.until(
            EC.element_to_be_clickable((
                By.CSS_SELECTOR, 
                '.board-modal-header-close[aria-label="Close"]'
            ))
        ).click()
        
        wait.until(EC.element_to_be_clickable((By.CSS_SELECTOR, '.share'))).click()
        
        wait.until(
            EC.element_to_be_clickable((
                By.CSS_SELECTOR, 
                '.share-menu-tab-selector-component > div:first-child'
            ))
        ).click()
                
        pgn_contents = wait.until(
            EC.presence_of_element_located((
                By.CSS_SELECTOR, 
                "textarea.share-menu-tab-pgn-textarea"
            ))
        )
        return pgn_contents.get_property('value')

    except Exception as e:
        logger.exception(f"Error getting PGN for game {game_id}: {str(e)}")
        return None


def get_tournament_round_uri(tt_uri: str, round: int, page_no: int):
    return '/tournament/live/' + tt_uri  + f'?round={round}&pairings={page_no}'


def get_total_pages(soup) -> int:
    return soup.select('#pairings .index-pagination #pairings-pagination-bottom')[0].get('data-total-pages')
    

def get_game_ids_from_page(soup: bs4.BeautifulSoup):
    game_tags = soup.select('table.tournaments-live-view-pairings-table > tbody > tr > td > a')

    def get_id_from_url(tag):
        url = tag.get('href')
        parsed = urllib3.util.parse_url(url)
        return parsed.path.split('/')[-1]
        
    return list(set(map(get_id_from_url, game_tags)))


if __name__ == '__main__':
    main()
