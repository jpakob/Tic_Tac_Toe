#pragma once

#include <stdint.h>

/*
 * Difficulty levels for waehleZug():
 *   0 = easy:   random legal move
 *   1 = medium: wins immediately, blocks immediately, otherwise random
 *   2 = hard:   trained Q table (random choice among equally good moves)
 *   3 = perfect: minimax, never loses
 */
constexpr uint8_t STUFE_ZUFALL  = 0;
constexpr uint8_t STUFE_MITTEL  = 1;
constexpr uint8_t STUFE_SCHWER  = 2;
constexpr uint8_t STUFE_PERFEKT = 3;

/* Sets the seed of the random number generator (only used by levels 0, 1 and 2).
 * On the ESP32 call it e.g. with esp_random(), in tests use a fixed value. */
void aiSeed(uint32_t seed);

/* Returns the best move for the AI (O) using minimax, or -1 if no move is possible. */
int minimaxZug(const uint8_t brett[9]);

/* Picks a move for the AI (O) depending on the level, or -1 if no move is possible. */
int waehleZug(const uint8_t brett[9], uint8_t stufe);