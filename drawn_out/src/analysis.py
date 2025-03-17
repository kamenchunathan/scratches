import io
import os
import re
from typing import Optional

import chess.pgn
import libsql_experimental as libsql
import pandas as pd


ROOK_ENDGAME_PIECES = ['r', 'k', 'K', 'R']


def analyze_game(pgn: str):
    game = chess.pgn.read_game(io.StringIO(pgn))
        
    board = game.end().board()
    
    (is_rook_endgame, moves_played_out) = moves_played_if_is_rook_endgame(board)
    if not is_rook_endgame:
        return
    
    outcome = board.outcome(claim_draw=True)
    outcome_str = None
    winner = None
        
    if outcome:
        outcome_str = outcome.termination.name 
        if outcome.winner is not None: # draws are none
            winner = 'w' if outcome.winner else 'b'
    # Game ended via reason that cannot be determined from the board e.g. 
    # flagging / resignation
    else:
        match game.headers.get('Result'):
            case '1-0':
                winner = 'w'
            case '0-1':
                winner = 'b'
    
    return {
        "rook_endgame": True,
        "draw_reason": outcome_str,
        "winner": winner,
        "played_out": moves_played_out
    }


def moves_played_if_is_rook_endgame(board: chess.Board) -> bool:
    def check_is_rook_endgame(board_fen: str) -> bool:
        """Checks whether remaining pieces are either a King or a rook"""
        return all(p in ROOK_ENDGAME_PIECES for p in re.findall(r'[a-zA-Z]', board_fen))
    
    moves = 0
    is_rook_endgame = False
    last_move = None
    while True:
        if not check_is_rook_endgame(board.board_fen()):
            # First iteration is false meaning it's not a rook endgame
            if moves == 0:
                is_rook_endgame = False
            else:
                # The 
                # Check that the next board position has 4 pieces: King and rook of both colors
                board.push(last_move)
                is_rook_endgame =  \
                    ( sum(board.pieces(chess.ROOK, chess.WHITE).tolist()) \
                        + sum(board.pieces(chess.ROOK, chess.BLACK).tolist()) \
                        + sum(board.pieces(chess.KING, chess.WHITE).tolist()) \
                        + sum(board.pieces(chess.KING, chess.BLACK).tolist()) \
                    ) == 4
            break
        
        moves +=1
        last_move = board.pop()
    
    return is_rook_endgame, moves
    

def analyze_data(df: pd.DataFrame):
    analyzed_df = df['pgn'].apply(lambda row: pd.Series(analyze_game(row))),
    if analyzed_df[0].empty:
        return pd.DataFrame(
            columns= [ "rook_endgame", "draw_reason", "winner", "played_out" ]
        )
    return df.join(analyzed_df[0]).dropna(
        how="all", 
        subset=["rook_endgame", "draw_reason", "winner", "played_out"]
    )
    print(res)


def fetch_data(db_uri: str, db_token: str, tournament_id):
    db = libsql.connect(db_uri, auth_token=db_token)
    return db.execute(
        """SELECT id, round, pgn, tournament_id FROM game WHERE tournament_id = ? AND status = 'success';""",
        (tournament_id,)
    ).fetchall()


def main():
    DB_URI = os.getenv("DB_URI")
    DB_TOKEN = os.getenv("DB_TOKEN")
    
    if DB_URI is None or DB_TOKEN is None:
        print('Required environement variables are set in the environment')
        return
    
    db = libsql.connect(DB_URI, auth_token=DB_TOKEN)
    completed_tournaments = db.execute(
        "SELECT tournament_id FROM jobs WHERE status = 'complete';"
    ).fetchall()

    total = pd.concat(
        analyze_data(
            pd.DataFrame(
                fetch_data(DB_URI, DB_TOKEN, tournament_id[0]), 
                columns=["id", "round", "pgn", "tournament_id"]
            ).set_index("id")
        )
        for tournament_id in completed_tournaments
    )
    
    with open('data/analyzed.csv', 'w') as f:
        total.to_csv(f)
 

if __name__ == '__main__':
    main()
