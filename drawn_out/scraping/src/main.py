import http.client
import os
import time
import urllib3
import logging
import json
from time import sleep
from typing import Optional, Tuple
from datetime import datetime

import asyncio
import bs4
import libsql_experimental as libsql
from playwright.async_api import async_playwright, Page, Browser, BrowserContext

DB_URI = os.getenv("DB_URI")
DB_TOKEN = os.getenv("DB_TOKEN", '')
PLAYWRIGHT_URI = os.getenv("PLAYWRIGHT_URI", "localhost")
MAX_TABS = int(os.getenv("MAX_TABS", "11"))


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


# Global tab pool management
semaphore = None
context_queue = None


# TODO: Flesh out this function to connect to playwright. This is currently unhandled 
# because we are now using a connection pool
def connect_to_playwright():
    """Connect to playwright host"""
    pass


async def initialize_browser_pool(browser: Browser, max_tabs: int):
    """Initialize the browser context pool"""
    global semaphore, context_queue
    semaphore = asyncio.Semaphore(max_tabs)
    context_queue = asyncio.Queue()
    
    # Create and add browser contexts to the queue
    for _ in range(max_tabs):
        context = await browser.new_context()
        page = await context.new_page()
        await context_queue.put({"context": context, "page": page})



async def get_context():
    """Get a browser context from the pool"""
    global semaphore, context_queue
    async with semaphore:
        context = await context_queue.get()
        return context


async def release_context(ctx: {"context": BrowserContext, "page": Page }):
    """Release a browser context back to the pool"""
    global context_queue
    await context_queue.put(ctx)


def libsql_connect() -> libsql.Connection:
    """Wrapper for database connection that tracks metrics"""
    return libsql.connect(DB_URI, auth_token=DB_TOKEN)


def run_migrations(db: libsql.Connection):
    logger.info("Running database migrations")
    with open('./migrations/init.sql') as f:
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
          COUNT(*) 
        FROM 
          game AS g 
          LEFT JOIN tournament AS t ON g.tournament_id = t.id 
        WHERE 
          t.id = ?
          AND NOT (
            g.status == 'success' 
            OR (
              g.status == 'error' 
              AND g.retries >= 3
            )
          );""",
        (tournament_id,)
    ).fetchone()
    if res is None:
        logger.error(f"Could not check job {tournament_id} for completeness")
        return
    unprocessed_games = res[0] 
    
    if rounds_fetched > 3 and unprocessed_games == 0:
        logger.info(f"Setting {tournament_id} as complete")
        db.execute("UPDATE jobs SET status = 'complete' WHERE tournament_id = ?;", (tournament_id,))
        db.commit()


def get_tournament_round_uri(tt_uri: str, round: int, page_no: int):
    return '/tournament/live/' + tt_uri  + f'?round={round}&pairings={page_no}'


def get_game_ids_from_page(soup: bs4.BeautifulSoup):
    game_tags = soup.select('table.tournaments-live-view-pairings-table > tbody > tr > td > a')

    def get_id_from_url(tag):
        url = tag.get('href')
        parsed = urllib3.util.parse_url(url)
        return parsed.path.split('/')[-1]
        
    return list(set(map(get_id_from_url, game_tags)))


def get_total_pages(soup) -> int:
    pagination_elements = soup.select('#pairings .index-pagination #pairings-pagination-bottom')
    if not pagination_elements:
        return 1
    return int(pagination_elements[0].get('data-total-pages', 1))


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
        page_count = get_total_pages(soup)
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


async def get_pgn(page: Page, game_id: str) -> Optional[str]:
    try:
        await page.goto(f'https://www.chess.com/game/live/{game_id}', wait_until='domcontentloaded')
        
        # Wait for and close the modal
        await page.wait_for_selector('.board-modal-header-close[aria-label="Close"]', timeout=120000)
        # Small delay to account for layout shifts
        await asyncio.sleep(2)
        await page.click('.board-modal-header-close[aria-label="Close"]')
        
        # Click share button
        await page.click('.share')
        
        # Select PGN tab
        await page.click('.share-menu-tab-selector-component > div:first-child')
        
        # Add timing information
        await page.click('span.circle-clock.icon-font-chess.share-menu-tab-pgn-icon')
        
        # Get PGN content
        pgn_element = await page.wait_for_selector("textarea.share-menu-tab-pgn-textarea")
        pgn = await pgn_element.input_value()
        
        return pgn

    except Exception as e:
        logging.error(f"Error fetching PGN for game {game_id}: {str(e)}")
        return None



async def populate_tournament_rounds(tournament_id: str):
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
            """INSERT INTO game (id, tournament_id, round, pgn) VALUES (?, ?, ?, '') ON CONFLICT DO NOTHING;""",
	        list(map(lambda game_id: (game_id, tournament_id, current_round), round_ids))
	    )
        db.execute(
            "UPDATE tournament SET rounds_fetched = rounds_fetched + 1 WHERE id = ?", 
            ( tournament_id, )
        ).fetchone()
        db.commit()


async def process_game(game_id: str, tournament_id: str, local_retry_tracker: dict[str, int], max_retries: int = 3):
    """Process a single game with local retries"""
    if game_id not in local_retry_tracker:
        local_retry_tracker[game_id] = 0
    
    success = False
    
    while local_retry_tracker[game_id] < max_retries and not success:
        # Get a browser context from the pool
        ctx = await get_context()
        context = ctx.get('context')
        page = ctx.get('page')
        try:
            start_time = time.time()
            pgn = await get_pgn(page, game_id)
            
            if pgn is not None:
                db = libsql_connect()
                db.execute(
                    """UPDATE game SET status = 'success', pgn = ?2 WHERE id = ?1;""", 
                    (game_id, pgn)
                )
                db.commit()
                success = True
            else:
                raise Exception()
        except:
            # Increment local retry count on failure
            local_retry_tracker[game_id] += 1
            # Wait before retry with exponential backoff
            backoff_time = 2 ** local_retry_tracker[game_id]
            await asyncio.sleep(backoff_time)
            
        finally:
            # Return the context to the pool
            await release_context({'context':context, 'page':page})
                

    # Update DB status only after all local retries are exhausted
    if not success:
        db = libsql_connect()
        db.execute( 
            """UPDATE game SET status = 'error', retries = retries + 1 WHERE id = ?;""",
            (game_id,)
        )
        db.commit()


async def populate_game_pgns(tournament_id: str, local_max_retries: int = 3):
    """
    Populates game PGNs for a tournament with local retries.
    
    Args:
        tournament_id: The ID of the tournament to populate PGNs for
        local_max_retries: Maximum number of local retries before updating database status
    """
    logger.info(f"Populating game PGNs for tournament {tournament_id}")
    
    # Local retry tracking dictionary
    local_retry_tracker = {}  # game_id -> retry_count
    
        
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
                AND (
                    status = 'not_asked' 
                    OR ( status = 'error' AND retries < 3)
                )
            LIMIT 50;""",
            (tournament_id,)
        ).fetchall()

        # Process batch of games with local retry mechanism
        tasks = []
        for (game_id,) in res:
            task = asyncio.create_task(
                process_game(
                    game_id, 
                    tournament_id, 
                    local_retry_tracker, 
                    local_max_retries
                )
            )
            tasks.append(task)
        
        # Wait for all tasks to complete
        if tasks:
            await asyncio.gather(*tasks)
            
        # Clean up local retry tracker to prevent memory growth
        for game_id in list(local_retry_tracker.keys()):
            if local_retry_tracker[game_id] >= local_max_retries:
                local_retry_tracker.pop(game_id, None)

    logger.info(f"Batch of games processed, logging interim metrics")
    log_metrics()


async def async_main():
    logger.info('Starting data collection process')
    # if DB_URI is None or DB_TOKEN is None:
    #     logging.error("Missing required environment variables")
    #     return

    db = libsql_connect()
    run_migrations(db)
    
    async with async_playwright() as p:
        browser = await p.chromium.connect(PLAYWRIGHT_URI)
        await initialize_browser_pool(browser, MAX_TABS)
        
        try:
            while True:
                tournament_id = fetch_current_job(db)
                print('current job', tournament_id)
                if tournament_id is None:
                    break

                await populate_tournament_rounds(tournament_id)
                await populate_game_pgns(tournament_id)

                # Check for completion of a job and update
                check_and_set_completed(tournament_id)
        
        finally:
            # Close all contexts and the browser
            while not context_queue.empty():
                context = await context_queue.get()
                await context.close()
            
            await browser.close()

    log_metrics()  # Log final metrics summary


def main():
    asyncio.run(async_main())


if __name__ == '__main__':
    main()
