import http.client
import os
import time
import urllib3
from typing import Optional, Tuple

import bs4
import libsql_experimental as libsql
from selenium import webdriver
from selenium.common.exceptions import NoSuchElementException
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options

DB_URI = os.getenv("DB_URI")
DB_TOKEN = os.getenv("DB_TOKEN")
HOST = 'www.chess.com'
TT_ROUNDS = 11

def main():
    if DB_URI is None or DB_TOKEN is None:
        print('Ensure DB_URI and DB_TOKEN are set in the environemnt')
        return

    db = libsql.connect(DB_URI, auth_token=DB_TOKEN)
    run_migrations(db)
    
    
    tournament_id = fetch_current_job(db)
    if tournament_id is None:
        # Assume there are no incomplete jobs
        # TODO: Log it and close program
        db.close()
        return

    print(tournament_id)
    populate_tournament_rounds(tournament_id)
    populate_game_pgns(tournament_id)
    db.close()


def run_migrations(db: libsql.Connection):
    with open('migrations/init.sql') as f:
        db.executescript(f.read())
        db.commit()
   
 
def fetch_current_job(db: libsql.Connection) -> Optional[str]:
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
    
    if pending_jobs is not None:
        return pending_jobs[0]
    
    return None


def populate_tournament_rounds(tournament_id: str):
    db = libsql.connect(DB_URI, auth_token=DB_TOKEN)
    res = db.execute(
        "SELECT rounds_fetched FROM tournament WHERE id = ?1;", 
        ( tournament_id, )
    ).fetchone()
    
    if res is None:
        # TODO: log the error
        return
    rounds_fetched = res[0]
    
    current_round = rounds_fetched
    for i in range(current_round, TT_ROUNDS + 1):
        # In between http / long running processes we create a new connection
        round_ids =  get_game_ids(tournament_id, current_round)
        db = libsql.connect(DB_URI, auth_token=DB_TOKEN)
        db.executemany("""
            INSERT INTO game ("id", "tournament_id", "round", "pgn")
            VALUES
	            (?, ?, ?, "")
	        ON CONFLICT DO NOTHING;""",
	        list(map(lambda game_id: (game_id, tournament_id, current_round), round_ids))
	    )
        db.execute(
            "UPDATE tournament SET rounds_fetched = rounds_fetched + 1 WHERE id = ?", 
            ( tournament_id, )
        ).fetchone()
        db.commit()


def populate_game_pgns(tournament_id: str):
    while True:
        db = libsql.connect(DB_URI, auth_token=DB_TOKEN)
        res = db.execute("""
            SELECT COUNT(*) AS matching_count
            FROM game
            WHERE
                tournament_id = ?
                AND (status = 'not_asked' OR status = 'error')
                AND retries < 3;""",
            ( tournament_id, )
        ).fetchone()
        if res is None:
            return
        count = res[0]
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
            ( tournament_id, )
        ).fetchall()
        
        options = Options()
        options.add_argument("--user-data-dir=./data/chromedriver")
        driver = webdriver.Chrome(options=options)

        for (game_id,) in res:
            pgn = get_pgn(driver, game_id)
            if pgn is not None:
                db.execute(
                    """UPDATE game SET status = 'success', pgn = ?2 WHERE id = ?1;""", 
                    ( game_id, pgn )
                )
            else:
                db.execute( 
                    """UPDATE game SET status = 'error', retries = retries + 1 WHERE id = ?;""",
                    ( game_id, )
                )
                            
            db.commit()
        
        driver.close()


def get_game_ids(tournament_id: str, tournament_round: int) -> [str]:
    game_ids = []
    
    current_page = 1
    conn = http.client.HTTPSConnection(HOST)
    conn.request(
        'GET', 
        get_tourndament_round_uri(
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
        pass
     
    if page_count <= 1:
        return game_ids
    
    for i in range(2, page_count + 1):
        conn.request(
            'GET', 
            get_tourndament_round_uri(
                tournament_id, 
                tournament_round , 
                current_page
            )
        )
        resp = conn.getresponse()
        soup = bs4.BeautifulSoup(resp.read(), 'html.parser')
        game_ids.extend(get_game_ids_from_page(soup))

    return game_ids


def get_pgn(driver: webdriver.chrome.webdriver.WebDriver, game_id: str, page_delay=10) -> Optional[str] :
    try:
        driver.get(f'https://www.chess.com/game/live/{game_id}')
        # Necessary to allow chess.com to load the modal
        time.sleep(page_delay)
                                
        close_modal_btn = driver.find_element(
            By.CSS_SELECTOR, 
            '.board-modal-header-close[aria-label="Close"'
        )
        close_modal_btn.click()
        
        share_btn = driver.find_element(By.CSS_SELECTOR, '.share')
        share_btn.click()
        time.sleep(2)
        
        pgn_tab = driver.find_element(
            By.CSS_SELECTOR, 
            '.share-menu-tab-selector-component > div:first-child'
        )
        pgn_tab.click()
        
        pgn_contents = driver.find_element(
            By.CSS_SELECTOR, 
            "textarea.share-menu-tab-pgn-textarea"
        )
        return pgn_contents.get_property('value')
    except:
        return

 
def get_tourndament_round_uri(tt_uri: str, round: int, page_no: int):
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

