"""Shared game logic and state encoding for the Tic-Tac-Toe AI.

The encoding must be identical to encodeState() in firmware/src/game.cpp:
index = sum(value(i) * 3**i), cells 0-8 row by row from the top left,
0 = empty, 1 = player (X), 2 = AI (O).
"""

PLAYER = 1
AI = 2
NUM_STATES = 3 ** 9   # 19683

LINES = [
    (0, 1, 2), (3, 4, 5), (6, 7, 8),   # rows
    (0, 3, 6), (1, 4, 7), (2, 5, 8),   # columns
    (0, 4, 8), (2, 4, 6),              # diagonals
]

POW3 = [3 ** i for i in range(9)]


def encode(board):
    """Converts a board (list of 9 values) into its state index 0..19682."""
    return sum(v * p for v, p in zip(board, POW3))


def decode(index):
    """Converts a state index back into a board (inverse of encode)."""
    board = []

    for _ in range(9):
        board.append(index % 3)
        index //= 3

    return board


def winner(board):
    """Returns 1 or 2 if that player has three in a row, otherwise 0."""
    for a, b, c in LINES:
        if board[a] != 0 and board[a] == board[b] == board[c]:
            return board[a]

    return 0


def legal_moves(board):
    """Returns the indices of all empty cells."""
    return [i for i in range(9) if board[i] == 0]


def view(board, player):
    """Returns the board as seen by 'player': that player always appears as 2.

    For the AI (2) the board is unchanged, for the player (1) the values
    1 and 2 are swapped. The result must not be modified by the caller.
    """
    if player == AI:
        return board

    return [0 if v == 0 else 3 - v for v in board]


def best_moves(q_row, moves, tolerance=1e-6):
    """Returns all moves whose Q value is (nearly) the maximum among 'moves'."""
    best = max(q_row[m] for m in moves)

    return [m for m in moves if q_row[m] >= best - tolerance]
