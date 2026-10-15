#include "game.h"

static const uint8_t LINES[8][3] = {
    {0,1,2}, {3,4,5}, {6,7,8},      // Zeilen
    {0,3,6}, {1,4,7}, {2,5,8},      // Spalten
    {0,4,8}, {2,4,6}                // Diagonalen
};

bool winLine(const uint8_t b[9], uint8_t line[3]) {
    for (const auto& l : LINES) {
        if (b[l[0]] != 0 && b[l[0]] == b[l[1]] && b[l[1]] == b[l[2]]) {
            line[0] = l[0]; line[1] = l[1]; line[2] = l[2];
            return true;
        }
    }
    return false;
}

uint8_t winner(const uint8_t b[9]) {
    uint8_t line[3];
    return winLine(b, line) ? b[line[0]] : 0;
}

bool isFull(const uint8_t b[9]) {
    for (uint8_t i = 0; i < 9; i++) {
        if (b[i] == 0) return false;
    }
    return true;
}

uint8_t legalMoves(const uint8_t b[9], uint8_t out[9]) {
    uint8_t n = 0;
    for (uint8_t i = 0; i < 9; i++) {
        if (b[i] == 0) out[n++] = i;
    }
    return n;
}

uint16_t encodeState(const uint8_t b[9]) {
    uint16_t index = 0, pot = 1;
    for (uint8_t i = 0; i < 9; i++) {
        index += b[i] * pot;
        pot *= 3;
    }
    return index;
}