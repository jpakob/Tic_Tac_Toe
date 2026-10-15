#pragma once
#include <stdint.h>

// Brett: 9 Felder, Index 0-8 zeilenweise ab oben links
// Wert: 0 = leer, 1 = Spieler (X), 2 = KI (O)

uint8_t  winner(const uint8_t b[9]);                  // 0 = keiner, 1 = Spieler, 2 = KI
bool     isFull(const uint8_t b[9]);
uint8_t  legalMoves(const uint8_t b[9], uint8_t out[9]);   // schreibt freie Felder, gibt Anzahl zurück
uint16_t encodeState(const uint8_t b[9]);             // Summe Wert(i) * 3^i, 0..19682
bool     winLine(const uint8_t b[9], uint8_t line[3]); // Gewinnlinie, falls vorhanden