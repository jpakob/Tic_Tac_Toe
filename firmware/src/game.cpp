#include "game.h"

/**
 * @brief All eight possible winning lines (3 rows, 3 columns, 2 diagonals).
 *
 * Each entry holds the three cell indices of one line.
 */
static const uint8_t LINES[8][3] =
{
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},   // rows
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},   // columns
    {0, 4, 8}, {2, 4, 6}               // diagonals
};

/**
 * @brief Finds a completed line on the board.
 *
 * Checks all eight winning lines. A line counts as complete if its three
 * cells are not empty and hold the same value.
 *
 * @param b     Board with 9 cells (0 = empty, 1 = player, 2 = AI).
 * @param line  Output: the three cell indices of the winning line.
 *              Only written if a line was found.
 * @return true if a completed line was found, otherwise false.
 */
bool winLine(const uint8_t b[9], uint8_t line[3])
{

    for(const auto& l : LINES)
    {
        if(b[l[0]] != 0 && b[l[0]] == b[l[1]] && b[l[1]] == b[l[2]])
        {
            line[0] = l[0];
            line[1] = l[1];
            line[2] = l[2];
            return true;
        }
    }

    return false;
}

/**
 * @brief Determines the winner of the game.
 *
 * @param b  Board with 9 cells (0 = empty, 1 = player, 2 = AI).
 * @return 1 if the player has won, 2 if the AI has won, 0 if nobody has won.
 */
uint8_t winner(const uint8_t b[9])
{

    uint8_t line[3];

    if(winLine(b, line))
    {
        return b[line[0]];
    }

    return 0;
}

/**
 * @brief Checks whether the board is completely filled.
 *
 * Together with winner() == 0 this means a draw.
 *
 * @param b  Board with 9 cells.
 * @return true if no cell is empty, otherwise false.
 */
bool isFull(const uint8_t b[9])
{

    for(uint8_t i = 0; i < 9; i++)
    {
        if(b[i] == 0)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Collects all empty cells, i.e. all moves that are allowed.
 *
 * @param b    Board with 9 cells.
 * @param out  Output: indices of the empty cells in ascending order.
 *             Must have room for 9 entries.
 * @return Number of empty cells written to out.
 */
uint8_t legalMoves(const uint8_t b[9], uint8_t out[9])
{

    uint8_t n = 0;

    for(uint8_t i = 0; i < 9; i++)
    {
        if(b[i] == 0)
        {
            out[n++] = i;
        }
    }

    return n;
}

/**
 * @brief Converts the board into a unique state index.
 *
 * The index is the sum of value(i) * 3^i over all cells, so every board
 * position gets exactly one number between 0 and 19682. The Python training
 * script and the firmware must use exactly this encoding.
 *
 * @param b  Board with 9 cells (0 = empty, 1 = player, 2 = AI).
 * @return State index in the range 0..19682.
 */
uint16_t encodeState(const uint8_t b[9])
{

    uint16_t index = 0;
    uint16_t pot = 1;   // 3^i

    for(uint8_t i = 0; i < 9; i++)
    {
        index += b[i] * pot;
        pot *= 3;
    }

    return index;
}