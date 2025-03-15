def drawn_out_game_info(pgn: str):
    game = chess.pgn.read_game(io.StringIO(pgn))
        
    end_node = game.end()
    board = end_node.board()

    if not is_rook_endgame(board.board_fen()):
        return {}
    
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
                
        
    moves_played_out = endgame_moves_played(board.copy())
    return  {
        'rook_endgame': is_rook_endgame(board.board_fen()),
        'draw_reason': outcome_str, 
        'winner': winner,
        'played_out': moves_played_out
    }


def endgame_moves_played(board: chess.Board) -> int:
    moves = 0
    while is_rook_endgame(board.board_fen()):
        moves +=1
        board.pop()
    return moves


def is_rook_endgame(board_fen: str) -> bool:
    return all(p in rook_endgame_pieces for p in re.findall(r'[a-zA-Z]', board_fen))
    


