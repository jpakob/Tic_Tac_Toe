#include <unity.h>

#include "game.h"
#include "ai.h"
#include "ai_table.h"

/**
 * @brief Unity hook, runs before every test. Not needed here.
 */
void setUp()
{

}

/**
 * @brief Unity hook, runs after every test. Not needed here.
 */
void tearDown()
{

}

/**
 * @brief The player wins with three in a row.
 */
void test_gewinn_reihe()
{

    uint8_t b[9] = {1,1,1, 0,0,0, 0,0,0};

    TEST_ASSERT_EQUAL_UINT8(1, winner(b));
}

/**
 * @brief No complete line means no winner.
 */
void test_kein_gewinner()
{

    uint8_t b[9] = {1,2,1, 0,0,0, 0,0,0};

    TEST_ASSERT_EQUAL_UINT8(0, winner(b));
}

/**
 * @brief The empty board has state index 0.
 */
void test_index_leer()
{

    uint8_t b[9] = {0};

    TEST_ASSERT_EQUAL_UINT16(0, encodeState(b));
}

/**
 * @brief All eight winning lines are detected for both players.
 */
void test_alle_gewinnlinien()
{

    const uint8_t L[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};

    for(int i = 0; i < 8; i++)
    {
        for(uint8_t p = 1; p <= 2; p++)
        {
            uint8_t b[9] = {0};

            b[L[i][0]] = p;
            b[L[i][1]] = p;
            b[L[i][2]] = p;

            TEST_ASSERT_EQUAL_UINT8(p, winner(b));
        }
    }
}

/**
 * @brief A full board without a winner is a draw.
 */
void test_remis()
{

    uint8_t b[9] = {1,2,1, 1,2,2, 2,1,1};

    TEST_ASSERT_TRUE(isFull(b));
    TEST_ASSERT_EQUAL_UINT8(0, winner(b));
}

/**
 * @brief The state encoding gives the expected indices, including the maximum.
 */
void test_index_kodierung()
{

    uint8_t a[9] = {1,0,0, 0,0,0, 0,0,0};
    uint8_t c[9] = {0,1,0, 0,0,0, 0,0,0};
    uint8_t d[9] = {2,2,2, 2,2,2, 2,2,2};

    TEST_ASSERT_EQUAL_UINT16(1, encodeState(a));
    TEST_ASSERT_EQUAL_UINT16(3, encodeState(c));
    TEST_ASSERT_EQUAL_UINT16(19682, encodeState(d));   // highest possible index
}

/**
 * @brief legalMoves() returns the number and positions of the empty cells.
 */
void test_legal_moves()
{

    uint8_t b[9] = {1,0,2, 0,0,0, 0,0,0};
    uint8_t m[9];

    TEST_ASSERT_EQUAL_UINT8(7, legalMoves(b, m));
    TEST_ASSERT_EQUAL_UINT8(1, m[0]);   // first empty cell is index 1
}

/**
 * @brief On an empty board there is no winning line.
 */
void test_winline_leer()
{

    uint8_t b[9] = {0};
    uint8_t line[3];

    TEST_ASSERT_FALSE(winLine(b, line));
}

/**
 * @brief Minimax takes an immediate win instead of blocking.
 *
 * O can win at cell 2, X threatens to win at cell 5. Winning comes first.
 */
void test_minimax_nimmt_gewinn()
{

    uint8_t b[9] = {2,2,0, 1,1,0, 1,0,0};

    TEST_ASSERT_EQUAL_INT(2, minimaxZug(b));
}

/**
 * @brief Minimax blocks an immediate win of the player.
 */
void test_minimax_blockt()
{

    uint8_t b[9] = {1,1,0, 0,2,0, 0,0,0};

    TEST_ASSERT_EQUAL_INT(2, minimaxZug(b));
}

/**
 * @brief Minimax returns -1 on a full board.
 */
void test_minimax_volles_brett()
{

    uint8_t b[9] = {1,2,1, 1,2,2, 2,1,1};

    TEST_ASSERT_EQUAL_INT(-1, minimaxZug(b));
}

/**
 * @brief Plays every possible game in which the player moves first.
 *
 * The player tries every free cell, the AI answers with the perfect level.
 * The player must never win.
 *
 * @param b  Board, restored before returning. Player (X) is to move.
 */
static void spieleAlleSpiele(uint8_t b[9])
{

    for(uint8_t i = 0; i < 9; i++)
    {
        if(b[i] != 0)
        {
            continue;
        }

        b[i] = 1;

        if(winner(b) == 1)
        {
            TEST_FAIL_MESSAGE("Der Spieler hat gegen Minimax gewonnen");
        }
        else if(!isFull(b))
        {
            int z = waehleZug(b, STUFE_PERFEKT);

            TEST_ASSERT_TRUE(z >= 0 && z < 9);
            TEST_ASSERT_EQUAL_UINT8(0, b[z]);

            b[z] = 2;

            if(winner(b) == 0 && !isFull(b))
            {
                spieleAlleSpiele(b);
            }

            b[z] = 0;
        }

        b[i] = 0;
    }
}

/**
 * @brief Minimax never loses, whatever order the player plays in.
 */
void test_minimax_verliert_nie()
{

    uint8_t b[9] = {0};

    spieleAlleSpiele(b);
}

/**
 * @brief Level 0 always returns a free cell.
 */
void test_stufe_zufall_legal()
{

    uint8_t b[9] = {1,0,2, 0,1,0, 0,0,2};

    for(uint32_t seed = 1; seed <= 50; seed++)
    {
        aiSeed(seed);

        int z = waehleZug(b, STUFE_ZUFALL);

        TEST_ASSERT_TRUE(z >= 0 && z < 9);
        TEST_ASSERT_EQUAL_UINT8(0, b[z]);
    }
}

/**
 * @brief Level 1 wins immediately and blocks, independent of the random seed.
 */
void test_stufe_mittel()
{

    uint8_t gewinn[9] = {2,2,0, 1,1,0, 1,0,0};   // O wins at 2
    uint8_t block[9]  = {1,1,0, 0,2,0, 0,0,0};   // O must block at 2

    for(uint32_t seed = 1; seed <= 20; seed++)
    {
        aiSeed(seed);

        TEST_ASSERT_EQUAL_INT(2, waehleZug(gewinn, STUFE_MITTEL));
        TEST_ASSERT_EQUAL_INT(2, waehleZug(block, STUFE_MITTEL));
    }
}

/**
 * @brief Plays every possible game in which the AI moves first.
 *
 * The AI answers with the perfect level, the player tries every free cell.
 * The player must never win.
 *
 * @param b  Board, restored before returning. AI (O) is to move.
 */
static void spieleAlleSpieleKiBeginnt(uint8_t b[9])
{

    int z = waehleZug(b, STUFE_PERFEKT);

    TEST_ASSERT_TRUE(z >= 0 && z < 9);
    TEST_ASSERT_EQUAL_UINT8(0, b[z]);

    b[z] = 2;

    if(winner(b) == 0 && !isFull(b))
    {
        for(uint8_t i = 0; i < 9; i++)
        {
            if(b[i] != 0)
            {
                continue;
            }

            b[i] = 1;

            if(winner(b) == 1)
            {
                TEST_FAIL_MESSAGE("The player won against minimax");
            }
            else if(!isFull(b))
            {
                spieleAlleSpieleKiBeginnt(b);
            }

            b[i] = 0;
        }
    }

    b[z] = 0;
}

/**
 * @brief Minimax never loses when it moves first either.
 */
void test_minimax_verliert_nie_ki_beginnt()
{

    uint8_t b[9] = {0};

    spieleAlleSpieleKiBeginnt(b);
}

/**
 * @brief Returns the cell with the highest Q value for a board (AI = O to move).
 *
 * @param b  Board with 9 cells.
 * @return Cell index of the best allowed move, or -1 if none is allowed.
 */
static int bestQMove(const uint8_t b[9])
{

    const int8_t* row = AI_Q[encodeState(b)];
    int best = -1;

    for(uint8_t i = 0; i < 9; i++)
    {
        if(row[i] == -128)
        {
            continue;
        }

        if(best < 0 || row[i] > row[best])
        {
            best = i;
        }
    }

    return best;
}

/**
 * @brief encodeState() gives the same indices as the Python encoding.
 */
void test_golden_encoding()
{

    for(uint8_t i = 0; i < 5; i++)
    {
        TEST_ASSERT_EQUAL_UINT16(AI_GOLDEN_INDEX[i], encodeState(AI_GOLDEN_BOARDS[i]));
    }
}

/**
 * @brief The trained table takes an immediate win.
 */
void test_q_table_takes_win()
{

    uint8_t b[9] = {2,2,0, 1,1,0, 1,0,0};

    TEST_ASSERT_EQUAL_INT(2, bestQMove(b));
}

/**
 * @brief The trained table blocks an immediate win of the player.
 */
void test_q_table_blocks()
{

    uint8_t b[9] = {1,1,0, 0,2,0, 0,0,0};

    TEST_ASSERT_EQUAL_INT(2, bestQMove(b));
}

/**
 * @brief Level "hard" takes an immediate win.
 */
void test_stufe_schwer_nimmt_gewinn()
{

    uint8_t b[9] = {2,2,0, 1,1,0, 1,0,0};

    TEST_ASSERT_EQUAL_INT(2, waehleZug(b, STUFE_SCHWER));
}

/**
 * @brief Level "hard" never loses against a random player.
 */
void test_stufe_schwer_verliert_nie()
{

    aiSeed(12345);

    uint32_t r = 987654321u;

    for(uint16_t spiel = 0; spiel < 2000; spiel++)
    {
        uint8_t b[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
        uint8_t amZug = (spiel % 2 == 0) ? 1 : 2;

        while(winner(b) == 0 && !isFull(b))
        {
            int z;

            if(amZug == 2)
            {
                z = waehleZug(b, STUFE_SCHWER);
            }
            else
            {
                uint8_t m[9];
                uint8_t n = legalMoves(b, m);

                r = r * 1664525u + 1013904223u;
                z = m[(r >> 16) % n];
            }

            b[z] = amZug;
            amZug = (uint8_t)(3 - amZug);
        }

        TEST_ASSERT_TRUE(winner(b) != 1);
    }
}

/**
 * @brief Entry point of the test program, runs all tests.
 *
 * @return Number of failed tests (0 = all passed).
 */
int main()
{

    UNITY_BEGIN();

    RUN_TEST(test_gewinn_reihe);
    RUN_TEST(test_kein_gewinner);
    RUN_TEST(test_index_leer);
    RUN_TEST(test_alle_gewinnlinien);
    RUN_TEST(test_remis);
    RUN_TEST(test_index_kodierung);
    RUN_TEST(test_legal_moves);
    RUN_TEST(test_winline_leer);

    RUN_TEST(test_minimax_nimmt_gewinn);
    RUN_TEST(test_minimax_blockt);
    RUN_TEST(test_minimax_volles_brett);
    RUN_TEST(test_minimax_verliert_nie);
    RUN_TEST(test_stufe_zufall_legal);
    RUN_TEST(test_stufe_mittel);

    RUN_TEST(test_minimax_verliert_nie_ki_beginnt);

    RUN_TEST(test_golden_encoding);
    RUN_TEST(test_q_table_takes_win);
    RUN_TEST(test_q_table_blocks);

    RUN_TEST(test_stufe_schwer_nimmt_gewinn);
    RUN_TEST(test_stufe_schwer_verliert_nie);

    return UNITY_END();

    return UNITY_END();
}