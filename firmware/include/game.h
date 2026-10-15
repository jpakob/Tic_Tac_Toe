#pragma once

#include <stdint.h>

/*
 * Board layout: 9 cells, index 0-8, row by row starting at the top left.
 *
 *   0 1 2
 *   3 4 5
 *   6 7 8
 *
 * Cell values: 0 = empty, 1 = player (X), 2 = AI (O)
 */

/* Returns the winner (0 = none, 1 = player, 2 = AI). */
uint8_t winner(const uint8_t b[9]);

/* Returns true if no empty cell is left. */
bool isFull(const uint8_t b[9]);

/* Writes all empty cells to out and returns how many there are. */
uint8_t legalMoves(const uint8_t b[9], uint8_t out[9]);

/* Returns the unique state index (0..19682) of the board. */
uint16_t encodeState(const uint8_t b[9]);

/* Returns true and writes the winning cells to line if a line is complete. */
bool winLine(const uint8_t b[9], uint8_t line[3]);