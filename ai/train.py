"""Trains the Q table by self-play and saves it as q_table.npy.

One table serves both roles: every move is learned from the point of view
of the player who moves (that player always appears as 2, see view()).
"""

import random
from pathlib import Path

import numpy as np

from common import NUM_STATES, best_moves, encode, legal_moves, view, winner

EPISODES = 500_000      # number of self-play games
ALPHA = 0.2             # learning rate
EPSILON_START = 0.3     # share of random moves at the beginning
EPSILON_END = 0.05      # share of random moves at the end
SEED = 1                # fixed seed, so a run can be repeated exactly
OUTPUT = Path(__file__).parent / "q_table.npy"


def train():
    """Runs the self-play training and returns the Q table."""
    random.seed(SEED)

    q = np.zeros((NUM_STATES, 9), dtype=np.float32)

    for episode in range(EPISODES):
        # Exploration shrinks linearly from EPSILON_START to EPSILON_END.
        epsilon = EPSILON_END + (EPSILON_START - EPSILON_END) * (1 - episode / EPISODES)

        board = [0] * 9
        player = random.choice([1, 2])   # who starts: both cases are trained

        while True:
            state = encode(view(board, player))
            moves = legal_moves(board)

            if random.random() < epsilon:
                action = random.choice(moves)
            else:
                action = random.choice(best_moves(q[state], moves))

            board[action] = player

            moves_after = legal_moves(board)
            finished = True

            if winner(board) == player:
                target = 1.0
            elif not moves_after:
                target = 0.0
            else:
                # The opponent moves next. What is good for the opponent
                # is bad for us, so the best opponent value is negated.
                next_state = encode(view(board, 3 - player))
                target = -max(q[next_state][m] for m in moves_after)
                finished = False

            q[state, action] += ALPHA * (target - q[state, action])

            if finished:
                break

            player = 3 - player

        if (episode + 1) % 100_000 == 0:
            print(f"episode {episode + 1}/{EPISODES}")

    return q


if __name__ == "__main__":
    table = train()
    np.save(OUTPUT, table)
    print(f"saved {OUTPUT}")
