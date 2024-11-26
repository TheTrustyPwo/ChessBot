from Chessnut import Game
import random
import logging

# Configure Debugging and Logging
DEBUG = False
logging.basicConfig(level=logging.DEBUG if DEBUG else logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


def debug(message):
    """Utility function for debugging."""
    if DEBUG:
        logging.debug(message)


# Constants
PIECE_VALUES = {
    "p": 1, "n": 3, "b": 3, "r": 5, "q": 9, "k": 0,
    "P": -1, "N": -3, "B": -3, "R": -5, "Q": -9, "K": 0
}
CENTER_SQUARES = {"d4", "d5", "e4", "e5"}
KING_ACTIVITY_SQUARES = {"c4", "c5", "d3", "d6", "e3", "e6", "f4", "f5"}

# Move history to reduce repetitive moves
MOVE_HISTORY = {}

# Define weights for move prioritization
WEIGHTS = {
    "central_control": 3,
    "captures": 8,
    "checks": 12,
    "defense": 7,
    "aggression": 10,
    "coordination": 15,
}


def apply_decay(move):
    """Apply a decay factor to repetitive moves."""
    if move in MOVE_HISTORY:
        MOVE_HISTORY[move] += 1
    else:
        MOVE_HISTORY[move] = 1
    return max(0, 10 - MOVE_HISTORY[move])  # Penalize repetitive moves


def fen_to_board(fen):
    """Convert FEN string to a board representation."""
    rows = fen.split()[0].split("/")
    board = {}
    for rank_idx, row in enumerate(rows):
        file_idx = 0
        for char in row:
            if char.isdigit():
                file_idx += int(char)
            else:
                square = f"{chr(file_idx + ord('a'))}{8 - rank_idx}"
                board[square] = char
                file_idx += 1
    return board


def evaluate_opponent_moves(game, moves):
    """Evaluate opponent's responses to each move."""
    opponent_moves = []
    for move in moves:
        g = Game(game.get_fen())
        g.apply_move(move)
        opponent_moves.extend(list(g.get_moves()))
    return set(opponent_moves)


def endgame_prioritization(board, moves, player):
    """Refine moves to corner the opponent king in the endgame."""
    prioritized_moves = []
    king_square = [sq for sq, piece in board.items() if piece.lower() == "k" and piece.isupper() != (player == "w")]

    for move in moves:
        end_square = move[2:4]

        # Target the opponent king directly
        if end_square in king_square:
            prioritized_moves.append((move, WEIGHTS["checks"] + 20))
            continue

        # Moves that restrict the opponent king's movement
        g = Game(board)
        g.apply_move(move)
        new_king_moves = list(g.get_moves())
        restricted_squares = len(king_square) - len(new_king_moves)

        score = WEIGHTS["coordination"] + restricted_squares
        prioritized_moves.append((move, score))

    # Sort by priority
    prioritized_moves.sort(key=lambda x: x[1], reverse=True)
    return [move for move, _ in prioritized_moves]


def prioritize_moves(game, moves, phase, board):
    """Prioritize moves dynamically based on game phase and opponent evaluation."""
    prioritized_moves = []
    player = "w" if game.get_fen().split()[1] == "w" else "b"

    for move in moves:
        start_square = move[:2]
        end_square = move[2:4]
        piece = board.get(start_square, " ")
        target_piece = board.get(end_square, " ")

        # Checkmate moves
        g = Game(game.get_fen())
        g.apply_move(move)
        if g.status == Game.CHECKMATE:
            debug(f"Checkmate move: {move}")
            return [move]

        # Captures
        if target_piece != " ":
            score = WEIGHTS["captures"] + PIECE_VALUES.get(target_piece.lower(), 0)
            prioritized_moves.append((move, score))
            continue

        # Central control
        if end_square in CENTER_SQUARES:
            score = WEIGHTS["central_control"]
            prioritized_moves.append((move, score))
            continue

        # Checks and Threats
        g = Game(game.get_fen())
        g.apply_move(move)
        if g.status == Game.CHECK:
            score = WEIGHTS["checks"]
            prioritized_moves.append((move, score))
            continue

        # Endgame: King Coordination
        if phase == "endgame":
            if piece.lower() in {"q", "r", "b"}:  # Aggressive long-range pieces
                score = WEIGHTS["coordination"] + apply_decay(move)
                prioritized_moves.append((move, score))
                continue

        # Default moves with decay
        score = apply_decay(move)
        prioritized_moves.append((move, score))

    # Sort moves by priority
    prioritized_moves.sort(key=lambda x: x[1], reverse=True)
    return [move for move, _ in prioritized_moves]


def chess_bot(obs):
    """Dynamic chess bot with prioritization and strategy."""
    fen = obs['board']
    game = Game(fen)
    moves = list(game.get_moves())

    if not moves:
        return None

    board = fen_to_board(fen)
    phase = "endgame" if len(moves) < 20 else "midgame"
    debug(f"Game phase: {phase}")

    opponent_moves = evaluate_opponent_moves(game, moves)
    debug(f"Evaluating opponent moves: {len(opponent_moves)} possibilities")

    prioritized_moves = prioritize_moves(game, moves, phase, board)
    best_move = prioritized_moves[0] if prioritized_moves else random.choice(moves)
    debug(f"Best move: {best_move}")
    return best_move


# Test the bot
if __name__ == "__main__":
    from kaggle_environments import make

    env = make("chess", debug=True)

    result = env.run(["sunfish1.py", "random"])
    print("Agent exit status/reward/time left: ")
    for agent in result[-1]:
        print("\t", agent.status, "/", agent.reward, "/", agent.observation.remainingOverageTime)
    env.render(mode="ipython", width=1000, height=1000)