#include <unity.h>

#include "ai.h"
#include "game.h"
#include "match.h"

static Match      m;
static MatchInput in;

/**
 * @brief Unity hook, runs before every test: fresh match, empty board, time 0.
 */
void setUp()
{

    aiSeed(1);
    matchInit(m, 1, 0);

    for(uint8_t i = 0; i < 9; i++)
    {
        in.occupied[i] = false;
    }

    in.buttonLevel   = false;
    in.buttonNewGame = false;
    in.nowMs         = 0;
}

/**
 * @brief Unity hook, runs after every test. Not needed here.
 */
void tearDown()
{

}

/**
 * @brief Sets the simulated time and runs one update. Button presses are consumed.
 *
 * @param nowMs  Simulated time in ms.
 */
static void step(uint32_t nowMs)
{

    in.nowMs = nowMs;
    matchUpdate(m, in);

    in.buttonLevel   = false;
    in.buttonNewGame = false;
}

/**
 * @brief Waits for the automatic start (player begins in the first game).
 *
 * @return Time in ms at which PLAYER_TURN was reached.
 */
static uint32_t startPlayerRound()
{

    step(0);
    step(READY_DWELL_MS);

    return READY_DWELL_MS;
}

/**
 * @brief Counts the cells of the board that hold a value.
 *
 * @param value  0 = empty, 1 = player, 2 = AI.
 * @return Number of cells with this value.
 */
static uint8_t countCells(uint8_t value)
{

    uint8_t n = 0;

    for(uint8_t i = 0; i < 9; i++)
    {
        if(m.board[i] == value)
        {
            n++;
        }
    }

    return n;
}

/**
 * @brief Finds the first cell that holds a value.
 *
 * @param value  0 = empty, 1 = player, 2 = AI.
 * @return Cell index, or -1 if there is none.
 */
static int findCell(uint8_t value)
{

    for(uint8_t i = 0; i < 9; i++)
    {
        if(m.board[i] == value)
        {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Overwrites the board of the match (to set up a position directly).
 *
 * @param b  Board with 9 cells.
 */
static void setBoard(const uint8_t b[9])
{

    for(uint8_t i = 0; i < 9; i++)
    {
        m.board[i] = b[i];
    }
}

/**
 * @brief The player puts a stone on a cell and the AI answers.
 *
 * @param cell  Cell of the player's stone.
 * @param t     Time of the player's move in ms.
 * @return Time in ms after the AI has answered (state PLAYER_TURN).
 */
static uint32_t playMove(uint8_t cell, uint32_t t)
{

    in.occupied[cell] = true;

    step(t);
    step(t + THINK_MAX_MS);

    return t + THINK_MAX_MS;
}

/**
 * @brief After the dwell time the game starts, not earlier.
 */
void test_ready_starts_after_dwell()
{

    step(0);

    TEST_ASSERT_TRUE(m.state == MatchState::READY);
    TEST_ASSERT_TRUE(m.message == MatchMessage::READY);

    step(READY_DWELL_MS - 1);

    TEST_ASSERT_TRUE(m.state == MatchState::READY);

    step(READY_DWELL_MS);

    TEST_ASSERT_TRUE(m.state == MatchState::PLAYER_TURN);
    TEST_ASSERT_TRUE(m.message == MatchMessage::YOUR_TURN);
}

/**
 * @brief With a stone on the board nothing starts, even after a long time.
 */
void test_ready_waits_for_empty_board()
{

    in.occupied[0] = true;

    step(0);

    TEST_ASSERT_TRUE(m.state == MatchState::READY);
    TEST_ASSERT_TRUE(m.message == MatchMessage::REMOVE_STONES);

    step(10000);

    TEST_ASSERT_TRUE(m.state == MatchState::READY);

    in.occupied[0] = false;

    step(10100);

    TEST_ASSERT_TRUE(m.state == MatchState::READY);
    TEST_ASSERT_TRUE(m.message == MatchMessage::READY);

    step(10100 + READY_DWELL_MS);

    TEST_ASSERT_TRUE(m.state == MatchState::PLAYER_TURN);
}

/**
 * @brief A press on the level button changes the level and delays the start.
 */
void test_level_button_restarts_dwell()
{

    step(0);

    TEST_ASSERT_EQUAL_UINT8(STUFE_MITTEL, m.level);

    in.buttonLevel = true;
    step(1500);

    TEST_ASSERT_EQUAL_UINT8(STUFE_SCHWER, m.level);

    step(READY_DWELL_MS);   // would have started without the button press

    TEST_ASSERT_TRUE(m.state == MatchState::READY);

    step(1500 + READY_DWELL_MS);

    TEST_ASSERT_TRUE(m.state == MatchState::PLAYER_TURN);
}

/**
 * @brief After the last level the button goes back to the first one.
 */
void test_level_wraps_around()
{

    for(uint8_t i = 0; i < 3; i++)   // 1 -> 2 -> 3 -> 0
    {
        in.buttonLevel = true;
        step(i);
    }

    TEST_ASSERT_EQUAL_UINT8(STUFE_ZUFALL, m.level);
}

/**
 * @brief A valid move is accepted, the AI answers within the thinking time.
 */
void test_valid_move_leads_to_ai_reply()
{

    uint32_t t = startPlayerRound();

    in.occupied[4] = true;
    step(t + 10);

    TEST_ASSERT_TRUE(m.state == MatchState::AI_THINKING);
    TEST_ASSERT_TRUE(m.message == MatchMessage::AI_THINKING);
    TEST_ASSERT_EQUAL_UINT8(1, m.board[4]);
    TEST_ASSERT_TRUE(m.pendingMove >= 0 && m.pendingMove != 4);

    step(t + 10 + THINK_MIN_MS - 1);   // too early

    TEST_ASSERT_TRUE(m.state == MatchState::AI_THINKING);

    step(t + 10 + THINK_MAX_MS);       // latest possible end

    TEST_ASSERT_TRUE(m.state == MatchState::PLAYER_TURN);
    TEST_ASSERT_EQUAL_UINT8(1, countCells(1));
    TEST_ASSERT_EQUAL_UINT8(1, countCells(2));
}

/**
 * @brief A stone on a cell of the AI is an error until it is removed.
 */
void test_cell_taken_is_fault()
{

    uint32_t t = startPlayerRound();

    t = playMove(4, t + 10);

    int aiCell = findCell(2);

    in.occupied[aiCell] = true;
    step(t + 10);

    TEST_ASSERT_TRUE(m.state == MatchState::FAULT);
    TEST_ASSERT_TRUE(m.message == MatchMessage::CELL_TAKEN);

    in.occupied[aiCell] = false;
    step(t + 20);

    TEST_ASSERT_TRUE(m.state == MatchState::PLAYER_TURN);
    TEST_ASSERT_TRUE(m.message == MatchMessage::YOUR_TURN);
}

/**
 * @brief Two stones at once are an error, both must be removed.
 */
void test_two_stones_is_fault()
{

    uint32_t t = startPlayerRound();

    in.occupied[0] = true;
    in.occupied[1] = true;
    step(t + 10);

    TEST_ASSERT_TRUE(m.state == MatchState::FAULT);
    TEST_ASSERT_TRUE(m.message == MatchMessage::ONE_STONE_ONLY);
    TEST_ASSERT_EQUAL_UINT8(0, countCells(1));   // no move was accepted

    in.occupied[0] = false;
    step(t + 20);

    TEST_ASSERT_TRUE(m.state == MatchState::FAULT);   // one stone is still there

    in.occupied[1] = false;
    step(t + 30);

    TEST_ASSERT_TRUE(m.state == MatchState::PLAYER_TURN);
}

/**
 * @brief A removed stone is an error until it is put back.
 */
void test_stone_missing_is_fault()
{

    uint32_t t = startPlayerRound();

    t = playMove(4, t + 10);

    in.occupied[4] = false;
    step(t + 10);

    TEST_ASSERT_TRUE(m.state == MatchState::FAULT);
    TEST_ASSERT_TRUE(m.message == MatchMessage::STONE_MISSING);

    in.occupied[4] = true;
    step(t + 20);

    TEST_ASSERT_TRUE(m.state == MatchState::PLAYER_TURN);
}

/**
 * @brief The winning move of the player ends the game and updates the score.
 */
void test_player_win_updates_score()
{

    uint32_t t = startPlayerRound();

    const uint8_t pos[9] = {1,1,0, 2,2,0, 0,0,0};

    setBoard(pos);

    in.occupied[0] = true;
    in.occupied[1] = true;
    in.occupied[2] = true;   // the new stone completes the top row
    step(t + 10);

    TEST_ASSERT_TRUE(m.state == MatchState::GAME_OVER);
    TEST_ASSERT_TRUE(m.message == MatchMessage::PLAYER_WINS);
    TEST_ASSERT_EQUAL_UINT16(1, m.scorePlayer);
    TEST_ASSERT_EQUAL_UINT16(0, m.scoreAi);
    TEST_ASSERT_TRUE(m.hasWinLine);
    TEST_ASSERT_EQUAL_UINT8(0, m.winLineCells[0]);
    TEST_ASSERT_EQUAL_UINT8(1, m.winLineCells[1]);
    TEST_ASSERT_EQUAL_UINT8(2, m.winLineCells[2]);
    TEST_ASSERT_FALSE(m.playerStartsNext);
}

/**
 * @brief After a game and an empty board the AI starts the next round.
 */
void test_ai_starts_next_round()
{

    uint32_t t = startPlayerRound();

    const uint8_t pos[9] = {1,1,0, 2,2,0, 0,0,0};

    setBoard(pos);

    in.occupied[0] = true;
    in.occupied[1] = true;
    in.occupied[2] = true;
    step(t + 10);   // game over

    step(t + 50);   // stones are still there: nothing changes

    TEST_ASSERT_TRUE(m.state == MatchState::GAME_OVER);

    in.occupied[0] = false;
    in.occupied[1] = false;
    in.occupied[2] = false;
    step(t + 100);

    TEST_ASSERT_TRUE(m.state == MatchState::READY);
    TEST_ASSERT_EQUAL_UINT8(9, countCells(0));   // board is empty again

    step(t + 100 + READY_DWELL_MS);

    TEST_ASSERT_TRUE(m.state == MatchState::AI_THINKING);
    TEST_ASSERT_TRUE(m.pendingMove >= 0);
}

/**
 * @brief A full board without a winner is a draw.
 */
void test_draw()
{

    uint32_t t = startPlayerRound();

    const uint8_t pos[9] = {1,2,1, 1,2,2, 2,1,0};

    setBoard(pos);

    in.occupied[0] = true;
    in.occupied[2] = true;
    in.occupied[3] = true;
    in.occupied[7] = true;
    in.occupied[8] = true;   // last free cell
    step(t + 10);

    TEST_ASSERT_TRUE(m.state == MatchState::GAME_OVER);
    TEST_ASSERT_TRUE(m.message == MatchMessage::DRAW);
    TEST_ASSERT_EQUAL_UINT16(1, m.scoreDraw);
    TEST_ASSERT_FALSE(m.hasWinLine);
}

/**
 * @brief The new-game button aborts a running game without changing the score.
 */
void test_new_game_button_aborts()
{

    uint32_t t = startPlayerRound();

    in.occupied[4] = true;
    step(t + 10);

    TEST_ASSERT_TRUE(m.state == MatchState::AI_THINKING);

    in.buttonNewGame = true;
    step(t + 20);

    TEST_ASSERT_TRUE(m.state == MatchState::READY);
    TEST_ASSERT_TRUE(m.message == MatchMessage::REMOVE_STONES);   // stone is still on cell 4
    TEST_ASSERT_EQUAL_UINT8(9, countCells(0));
    TEST_ASSERT_EQUAL_UINT16(0, m.scorePlayer + m.scoreAi + m.scoreDraw);
}

/**
 * @brief A stone put down while the AI thinks is ignored, then handled as a normal move.
 */
void test_sensors_ignored_while_thinking()
{

    uint32_t t = startPlayerRound();

    in.occupied[4] = true;
    step(t + 10);

    uint8_t other = (m.pendingMove == 0) ? 1 : 0;   // a free cell that is not the AI's choice

    in.occupied[other] = true;
    step(t + 20);

    TEST_ASSERT_TRUE(m.state == MatchState::AI_THINKING);
    TEST_ASSERT_EQUAL_UINT8(0, m.board[other]);

    step(t + 10 + THINK_MAX_MS);

    TEST_ASSERT_TRUE(m.state == MatchState::PLAYER_TURN);

    step(t + 20 + THINK_MAX_MS);

    TEST_ASSERT_TRUE(m.state == MatchState::AI_THINKING);   // the stone counted as a move
    TEST_ASSERT_EQUAL_UINT8(1, m.board[other]);
}

/**
 * @brief Entry point of the test program, runs all tests.
 *
 * @return Number of failed tests (0 = all passed).
 */
int main()
{

    UNITY_BEGIN();

    RUN_TEST(test_ready_starts_after_dwell);
    RUN_TEST(test_ready_waits_for_empty_board);
    RUN_TEST(test_level_button_restarts_dwell);
    RUN_TEST(test_level_wraps_around);
    RUN_TEST(test_valid_move_leads_to_ai_reply);
    RUN_TEST(test_cell_taken_is_fault);
    RUN_TEST(test_two_stones_is_fault);
    RUN_TEST(test_stone_missing_is_fault);
    RUN_TEST(test_player_win_updates_score);
    RUN_TEST(test_ai_starts_next_round);
    RUN_TEST(test_draw);
    RUN_TEST(test_new_game_button_aborts);
    RUN_TEST(test_sensors_ignored_while_thinking);

    return UNITY_END();
}