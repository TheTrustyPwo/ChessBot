import Chessnut
from Chessnut import Game
import random
from random import sample
import time

piece_sym = ['K', 'k', 'Q', 'q', 'R', 'r', 'B', 'b', 'N', 'n', 'P', 'p']
piece_val = [1000, 1000, 9, 9, 5, 5, 3, 3, 3, 3, 1, 1]
piece_val_dict = dict(zip(piece_sym, piece_val))

import chess
import numpy as np
import pickle
def extract_features(fen):
    board = chess.Board(fen)
    features = []

    # Material count (white pieces - black pieces)
    piece_sym = ['K', 'k', 'Q', 'q', 'R', 'r', 'B', 'b', 'N', 'n', 'P', 'p']
    piece_val = [1000, -1000, 9, -9, 5, -5, 3, -3, 3, -3, 1, -1]
    piece_val_dict = dict(zip(piece_sym, piece_val))
    val = 0
    for square in range(64):
        piece = board.piece_at(square)
        if piece is not None:
            val += piece_val_dict[piece.symbol()]
    features.append(val)

    # King safety
    def king_safety(king_square):
        if king_square is None:
            return 0
        rank, file = divmod(king_square, 8)
        nearby_squares = [
            chess.square(file + dx, rank + dy)
            for dx in (-1, 0, 1)
            for dy in (-1, 0, 1)
            if 0 <= file + dx < 8 and 0 <= rank + dy < 8
        ]
        return sum(1 for sq in nearby_squares if board.piece_at(sq) and board.piece_at(sq).piece_type == chess.PAWN)

    features.append(king_safety(board.king(chess.WHITE)))
    features.append(king_safety(board.king(chess.BLACK)))

    # Piece activity (number of legal moves)
    features.append(len(list(board.legal_moves)))

    # Pawn structure
    def pawn_structure(pawns, color, board):
        """
        Evaluate passed, doubled, and isolated pawns for a given color.
    
        Args:
            pawns (list): List of pawn squares for the given color.
            color (chess.Color): Color of the pawns (chess.WHITE or chess.BLACK).
            board (chess.Board): The chess board.
    
        Returns:
            tuple: (passed, doubled, isolated) counts.
        """
        passed = 0
        doubled = 0
        isolated = 0
    
        for pawn_square in pawns:
            rank, file = divmod(pawn_square, 8)
    
            # Passed pawns: no opposing pawns ahead on the same file or adjacent files
            if color == chess.WHITE:
                forward_squares = chess.BB_RANKS[rank + 1:]  # Squares ahead for white
            else:
                forward_squares = chess.BB_RANKS[:rank]  # Squares ahead for black
    
            adjacent_files = [file]
            if file > 0:
                adjacent_files.append(file - 1)  # Left file
            if file < 7:
                adjacent_files.append(file + 1)  # Right file
    
            is_passed = all(
                not board.pieces(chess.PAWN, not color) & chess.BB_FILES[f - 1] & n
                for n in forward_squares
                for f in adjacent_files
            )
            if is_passed:
                passed += 1
    
            # Doubled pawns: more than one pawn on the same file
            pawns_on_file = [sq for sq in pawns if chess.square_file(sq) == file]
            if len(pawns_on_file) > 1:
                doubled += 1
    
            # Isolated pawns: no friendly pawns on adjacent files
            is_isolated = all(
                not board.pieces(chess.PAWN, color) & chess.BB_FILES[f - 1]
                for f in range(file - 1, file + 2)
                if 0 <= f < 8 and f != file
            )
            if is_isolated:
                isolated += 1
    
        return passed, doubled, isolated

    white_pawns = list(board.pieces(chess.PAWN, chess.WHITE))
    black_pawns = list(board.pieces(chess.PAWN, chess.BLACK))

    white_passed, white_doubled, white_isolated = pawn_structure(white_pawns, chess.WHITE, board)
    black_passed, black_doubled, black_isolated = pawn_structure(black_pawns, chess.BLACK, board)

    features.extend([white_passed, black_passed])
    features.extend([white_doubled, black_doubled])
    features.extend([white_isolated, black_isolated])

    # Central control
    central_squares = [chess.D4, chess.E4, chess.D5, chess.E5]
    central_control_white = sum(1 for sq in central_squares if board.is_attacked_by(chess.WHITE, sq))
    central_control_black = sum(1 for sq in central_squares if board.is_attacked_by(chess.BLACK, sq))

    features.append(central_control_white - central_control_black)

    # return np.array([val, central_control_white - central_control_black])
    return np.array(features)

with open("chess_eval_model.pkl", "rb") as f:
    model = pickle.load(f)

def evaluate_position(board):
    example_features = extract_features(board.get_fen())
    return model.predict([example_features])[0]


# Minimax algorithm with alpha-beta pruning to evaluate and choose the best move for the bot.
def minimax(game, depth, maximizing_player, alpha=float('-inf'), beta=float('inf'), start_time=None, time_limit=None):
    if game.status in [Game.CHECKMATE, Game.STALEMATE]:
        return 0, None
    elif depth == 0:
        return evaluate_position(game), None
    elif start_time and time_limit and (time.time() - start_time) > time_limit:
        return evaluate_position(game), None

    # Limiting possible moves here, to save on search time.
    moves = sample(list(game.get_moves()), len(list(game.get_moves())) // 10)[:10] 
    best_move = None

    if maximizing_player:
        max_eval = float('-inf')
        for move in moves:
            g = Game(game.get_fen())
            g.apply_move(move)
            eval_score, _ = minimax(g, depth - 1, False, alpha, beta, start_time, time_limit)
            if eval_score > max_eval:
                max_eval = eval_score
                best_move = move
            alpha = max(alpha, eval_score)
            if beta <= alpha:
                break  # Beta cut-off
        return max_eval, best_move
    else:
        min_eval = float('inf')
        for move in moves:
            g = Game(game.get_fen())
            g.apply_move(move)
            eval_score, _ = minimax(g, depth - 1, True, alpha, beta, start_time, time_limit)
            if eval_score < min_eval:
                min_eval = eval_score
                best_move = move
            beta = min(beta, eval_score)
            if beta <= alpha:
                break  # Alpha cut-off
        return min_eval, best_move


default_fen = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'


def chess_bot(obs):
    # Set start_time
    start_time = time.time()
    time_limit = 0.10
    
    # Get FEN string from the Board object
    game = Game(obs.board)
    moves_full = list(game.get_moves())
    moves_sample = sample(moves_full, len(list(game.get_moves())))[:10]
    
    # 1. Try to detect checkmate
    for move in moves_full:
        g = Game(obs.board)
        g.apply_move(move)
        if g.status == Game.CHECKMATE:
            return move

    # 2. Check for captures of higher value piece using a lower value piece
    highest_val_capture = ''
    highest_val = 0
    for move in moves_full:
        moving_piece_val = piece_val_dict.get(game.board.get_piece(Game.xy2i(move[:2])), 0)
        captured_piece_val = piece_val_dict.get(game.board.get_piece(Game.xy2i(move[2:4])), 0)
        if captured_piece_val == moving_piece_val == 9:
            return move
        elif (captured_piece_val > moving_piece_val) and (captured_piece_val >= highest_val):
            highest_val_capture = move
            highest_val = captured_piece_val
    
    if highest_val_capture != '' or highest_val > 1:
        return highest_val_capture

    # 3. Check for captures
    for move in moves_sample:
        if game.board.get_piece(Game.xy2i(move[2:4])) != ' ':
            return move
    
    # 4. Use minimax algorithm with a depth of 1 to find the best move
    best_move = None
    best_eval = float('-inf')
    
    for move in moves_full:
        elapsed_time = time.time() - start_time
        if elapsed_time > float(time_limit):
            break
        
        g = Game(obs.board)
        g.apply_move(move)
        eval_score, _ = minimax(g, depth=3, maximizing_player=True, start_time=start_time, time_limit=time_limit)

        if eval_score > best_eval:
            best_eval = eval_score
            best_move = move
    
    if best_move is not None:
        return best_move
    else:
        return random.choice(moves_full)
