#include "ai.h"
#include "game.h"
#include "ai_table.h"

static const uint8_t SPIELER = 1;
static const uint8_t KI      = 2;

static uint32_t rngState = 2463534242u;

/**
 * @brief Small random number generator (xorshift32), independent of Arduino.
 *
 * This way the AI also runs in the native env on the Mac and is
 * reproducible in tests with a fixed seed.
 *
 * @return Next random number.
 */
static uint32_t naechsteZufallszahl()
{

    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;

    return rngState;
}

void aiSeed(uint32_t seed)
{

    if(seed == 0)
    {
        seed = 2463534242u;   // xorshift must not start with 0
    }

    rngState = seed;
}

/**
 * @brief Minimax evaluation of a position from the AI's point of view.
 *
 * AI win = 10 - tiefe, player win = tiefe - 10, draw = 0.
 * The depth makes a fast win preferable to a slow one and delays
 * a loss as long as possible.
 *
 * @param b      Board (is modified, but restored before returning).
 * @param amZug  Who moves next (1 or 2).
 * @param tiefe  Number of moves since the starting position.
 * @return Evaluation of the position.
 */
static int minimax(uint8_t b[9], uint8_t amZug, uint8_t tiefe)
{

    uint8_t w = winner(b);

    if(w == KI)
    {
        return 10 - tiefe;
    }

    if(w == SPIELER)
    {
        return tiefe - 10;
    }

    if(isFull(b))
    {
        return 0;
    }

    uint8_t naechster;
    int     best;

    if(amZug == KI)
    {
        naechster = SPIELER;
        best      = -100;   // AI maximizes
    }
    else
    {
        naechster = KI;
        best      = 100;    // player minimizes
    }

    for(uint8_t i = 0; i < 9; i++)
    {
        if(b[i] != 0)
        {
            continue;
        }

        b[i] = amZug;

        int score = minimax(b, naechster, tiefe + 1);

        b[i] = 0;

        if(amZug == KI)
        {
            if(score > best)
            {
                best = score;
            }
        }
        else
        {
            if(score < best)
            {
                best = score;
            }
        }
    }

    return best;
}

int minimaxZug(const uint8_t brett[9])
{

    uint8_t b[9];

    for(uint8_t i = 0; i < 9; i++)
    {
        b[i] = brett[i];
    }

    int bestZug   = -1;
    int bestScore = -100;

    for(uint8_t i = 0; i < 9; i++)
    {
        if(b[i] != 0)
        {
            continue;
        }

        b[i] = KI;

        int score = minimax(b, SPIELER, 1);

        b[i] = 0;

        if(score > bestScore)   // on a tie the first move is kept
        {
            bestScore = score;
            bestZug   = i;
        }
    }

    return bestZug;
}

/**
 * @brief Looks for a move that lets the given player win immediately.
 *
 * @param brett    Board with 9 cells.
 * @param spieler  1 or 2.
 * @return Cell index of the winning move, or -1 if there is none.
 */
static int sofortGewinnZug(const uint8_t brett[9], uint8_t spieler)
{

    uint8_t b[9];

    for(uint8_t i = 0; i < 9; i++)
    {
        b[i] = brett[i];
    }

    for(uint8_t i = 0; i < 9; i++)
    {
        if(b[i] != 0)
        {
            continue;
        }

        b[i] = spieler;

        bool gewinnt = (winner(b) == spieler);

        b[i] = 0;

        if(gewinnt)
        {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Picks one of the empty cells at random.
 *
 * @param brett  Board with 9 cells.
 * @return Cell index, or -1 if the board is full.
 */
static int zufallsZug(const uint8_t brett[9])
{

    uint8_t m[9];
    uint8_t n = legalMoves(brett, m);

    if(n == 0)
    {
        return -1;
    }

    return m[naechsteZufallszahl() % n];
}

/**
 * @brief Picks a move from the trained Q table.
 *
 * Among all moves with the highest Q value one is chosen at random,
 * so the AI does not play the same game every time.
 *
 * @param brett  Board with 9 cells (AI = 2 is the one to move).
 * @return Cell index, or -1 if no move is possible.
 */
static int qTabellenZug(const uint8_t brett[9])
{

    const int8_t* zeile = AI_Q[encodeState(brett)];
    int8_t        best  = -128;

    for(uint8_t i = 0; i < 9; i++)
    {
        if(zeile[i] > best)
        {
            best = zeile[i];
        }
    }

    if(best == -128)   // every cell is marked "not allowed"
    {
        return -1;
    }

    uint8_t m[9];
    uint8_t n = 0;

    for(uint8_t i = 0; i < 9; i++)
    {
        if(zeile[i] == best)
        {
            m[n] = i;
            n++;
        }
    }

    return m[naechsteZufallszahl() % n];
}

int waehleZug(const uint8_t brett[9], uint8_t stufe)
{

    if(stufe >= STUFE_PERFEKT)
    {
        return minimaxZug(brett);
    }

    if(stufe == STUFE_SCHWER)
    {
        return qTabellenZug(brett);
    }

    if(stufe == STUFE_MITTEL)
    {
        int z = sofortGewinnZug(brett, KI);       // 1. win right away

        if(z < 0)
        {
            z = sofortGewinnZug(brett, SPIELER);  // 2. block the player's win
        }

        if(z >= 0)
        {
            return z;
        }
    }

    return zufallsZug(brett);
}