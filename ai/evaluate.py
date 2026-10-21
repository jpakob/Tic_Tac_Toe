"""Checks the trained Q table.

1. Exhaustive check: every possible game is played through, once with the
   player starting and once with the AI starting. If the AI has several
   equally good moves, ALL of them are tried (worst case). The player must
   never win.
2. 10 000 random games against a player who moves randomly.
"""

import random
import sys
from pathlib import Path

import numpy as np

from common import AI, PLAYER, best_moves, encode, legal_moves, winner

Q_FILE = Path(__file__).parent / "q_table.npy"
RANDOM_GAMES = 10_000


def ai_candidates(q, board):
    """Returns all moves the AI might choose in this position (ties included)."""
    return best_moves(q[encode(board)], legal_moves(board))


def explore(q, board, to_move, stats):
    """Plays all possible games from this position and counts the results.

    The player tries every free cell, the AI tries every move it might pick.
    The board is restored before returning.
    """
    w = winner(board)

    if w == PLAYER:
        stats["player_wins"] += 1
        return

    if w == AI:
        stats["ai_wins"] += 1
        return

    moves = legal_moves(board)

    if not moves:
        stats["draws"] += 1
        return

    candidates = ai_candidates(q, board) if to_move == AI else moves

    for m in candidates:
        board[m] = to_move
        explore(q, board, 3 - to_move, stats)
        board[m] = 0


def play_random_game(q, ai_starts):
    """Plays one game: AI with random choice among its best moves vs random player.

    Returns the winner (0 = draw).
    """
    board = [0] * 9
    to_move = AI if ai_starts else PLAYER

    while True:
        if to_move == AI:
            move = random.choice(ai_candidates(q, board))
        else:
            move = random.choice(legal_moves(board))

        board[move] = to_move

        w = winner(board)

        if w != 0:
            return w

        if not legal_moves(board):
            return 0

        to_move = 3 - to_move


def main():
    q = np.load(Q_FILE)
    failed = False

    for name, first in (("player starts", PLAYER), ("AI starts", AI)):
        stats = {"player_wins": 0, "ai_wins": 0, "draws": 0}

        explore(q, [0] * 9, first, stats)

        total = sum(stats.values())
        print(f"{name}: {total} games -> {stats}")

        if stats["player_wins"] > 0:
            failed = True

    random.seed(2)
    results = {0: 0, 1: 0, 2: 0}

    for i in range(RANDOM_GAMES):
        results[play_random_game(q, ai_starts=(i % 2 == 0))] += 1

    print(f"random player: AI wins {results[2]}, draws {results[0]}, AI loses {results[1]}")

    if results[1] > 0:
        failed = True

    if failed:
        print("RESULT: FAILED - the AI can lose. Train longer (EPISODES).")
        sys.exit(1)

    print("RESULT: OK - the AI never loses.")


if __name__ == "__main__":
    main()
