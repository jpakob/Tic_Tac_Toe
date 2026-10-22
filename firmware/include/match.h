#pragma once

#include <stdint.h>

#include "ai.h"

/*
 * Game flow of one device, without any hardware access.
 *
 * Inputs come from the sensors and buttons (already debounced), outputs are
 * the public fields of struct Match, which the LED and LCD code only reads.
 * The states "start" and "calibrate" of the plan stay in main.cpp.
 */

enum class MatchState : uint8_t
{
    READY,         // between two games, waits for an empty board
    PLAYER_TURN,   // waits for the player's stone
    AI_THINKING,   // short pause, then the AI sets its O
    GAME_OVER,     // result is shown, waits until all stones are removed
    FAULT          // board does not match, waits until it is correct again
};

/* What the LCD should say. The mapping to texts is done in lcd_ui. */
enum class MatchMessage : uint8_t
{
    NONE,
    READY,
    REMOVE_STONES,
    YOUR_TURN,
    AI_THINKING,
    PLAYER_WINS,
    AI_WINS,
    DRAW,
    CELL_TAKEN,
    STONE_MISSING,
    ONE_STONE_ONLY
};

constexpr uint32_t READY_DWELL_MS = 2000;   // idle time in READY before the game starts
constexpr uint32_t THINK_MIN_MS   = 600;    // shortest "AI thinks" pause
constexpr uint32_t THINK_MAX_MS   = 1500;   // longest "AI thinks" pause
constexpr uint8_t  LEVEL_COUNT    = 4;      // STUFE_ZUFALL .. STUFE_PERFEKT

/* Inputs of one update step. Button flags are true for ONE update only. */
struct MatchInput
{
    bool     occupied[9];     // debounced: a stone lies on cell i
    bool     buttonLevel;     // difficulty button was pressed
    bool     buttonNewGame;   // new-game button was pressed
    uint32_t nowMs;           // current time (millis())
};

struct Match
{
    MatchState   state;
    MatchMessage message;
    uint8_t      board[9];            // 0 = empty, 1 = player, 2 = AI
    uint8_t      level;               // STUFE_ZUFALL .. STUFE_PERFEKT
    uint16_t     scorePlayer;
    uint16_t     scoreAi;
    uint16_t     scoreDraw;
    bool         playerStartsNext;    // who begins the next game
    bool         hasWinLine;          // valid in GAME_OVER after a win
    uint8_t      winLineCells[3];
    int8_t       pendingMove;         // cell the AI has chosen while it "thinks", else -1
    uint32_t     timer;               // end of the dwell time or the thinking pause
    uint32_t     rng;                 // random generator state for the thinking time
};

/* Resets everything to the start of a session (board empty, score 0). */
void matchInit(Match& m, uint32_t seed, uint32_t nowMs);

/* Call this in every loop pass. */
void matchUpdate(Match& m, const MatchInput& in);