from Chessnut import Game
import time
import random

PIECE_VALUES = {'p': 100, 'n': 320, 'b': 330, 'r': 500, 'q': 900, 'k': 20000}

def evaluate_board(game):
    """
    Evaluate the board position from the perspective of the current player.
    Positive scores are good for the current player, negative for the opponent.
    """
    board_str = str(game.board)
    score = 0
    for char in board_str:
        if char.isalpha():
            if char.isupper():
                score += PIECE_VALUES[char.lower()]
            else:
                score -= PIECE_VALUES[char]
    return score

def minimax(game, depth, alpha, beta, maximizing_player, start_time, time_limit):
    if time.time() - start_time > time_limit:
        return None, None
    if depth == 0 or game.status >= 2:
        return evaluate_board(game), None
    moves = list(game.get_moves())
    if not moves:
        return evaluate_board(game), None
    best_move = None
    if maximizing_player:
        max_eval = float('-inf')
        for move in moves:
            # Time check
            if time.time() - start_time > time_limit:
                break
            new_game = Game(str(game))
            new_game.apply_move(move)
            eval_score, _ = minimax(new_game, depth - 1, alpha, beta, False, start_time, time_limit)
            if eval_score is None:
                continue
            if eval_score > max_eval:
                max_eval = eval_score
                best_move = move
            alpha = max(alpha, eval_score)
            if beta <= alpha:
                break
        return max_eval, best_move
    else:
        min_eval = float('inf')
        for move in moves:
            # Time check
            if time.time() - start_time > time_limit:
                break
            new_game = Game(str(game))
            new_game.apply_move(move)
            eval_score, _ = minimax(new_game, depth - 1, alpha, beta, True, start_time, time_limit)
            if eval_score is None:
                continue
            if eval_score < min_eval:
                min_eval = eval_score
                best_move = move
            beta = min(beta, eval_score)
            if beta <= alpha:
                break
        return min_eval, best_move

def heuristic_chess_bot(obs):
    game = Game(obs.board)
    moves = list(game.get_moves())
    best_score = float('-inf')
    best_move = None
    for move in moves:
        move_score = 0
        from_square = move[:2]
        to_square = move[2:4]
        piece_moved = game.board.get_piece(Game.xy2i(from_square))
        target_piece = game.board.get_piece(Game.xy2i(to_square))
        # Add score for capturing a piece
        if target_piece != ' ':
            move_score += PIECE_VALUES[target_piece.lower()] - PIECE_VALUES[piece_moved.lower()]
        # Add a small bonus for developing pieces (moving from the back rank)
        if piece_moved.lower() in ['n', 'b', 'q']:
            if (piece_moved.isupper() and from_square[1] == '1') or (piece_moved.islower() and from_square[1] == '8'):
                move_score += 10
        # Check for promotions
        if "q" in move.lower():
            move_score += PIECE_VALUES['q']
        # Prefer castling
        if move in ['e1g1', 'e1c1', 'e8g8', 'e8c8']:
            move_score += 50
        if move_score > best_score:
            best_score = move_score
            best_move = move
    if best_move:
        return best_move
    else:
        return random.choice(moves)

def agent(obs, config):
    try:
        return hybrid_chess_bot(obs)
    except Exception:
        return random.choice(list(Game(obs.board).get_moves()))

def hybrid_chess_bot(obs):
    game = Game(obs.board)
    start_time = time.time()
    time_limit = 0.09  # Set per-move time limit to 0.09 seconds
    best_move = None
    moves = list(game.get_moves())
    if not moves:
        return "0000"  # No legal moves
    depth = 1
    try:
        # Limit depth to ensure completion within time limit
        while depth <= 2:
            elapsed_time = time.time() - start_time
            if elapsed_time >= time_limit:
                break
            current_time_limit = time_limit - elapsed_time
            eval_score, move = minimax(game, depth, float('-inf'), float('inf'), True, start_time, time_limit)
            if move:
                best_move = move
            depth += 1
    except Exception:
        pass
    if best_move:
        return best_move
    else:
        # Time is running out or no move found, use heuristic or random move
        elapsed_time = time.time() - start_time
        if elapsed_time < time_limit:
            return heuristic_chess_bot(obs)
        else:
            return random.choice(moves)