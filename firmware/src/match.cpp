#include "match.h"

#include "game.h"

static const uint8_t CELL_PLAYER = 1;
static const uint8_t CELL_AI     = 2;

/**
 * @brief Checks whether a point in time has been reached (safe on millis() overflow).
 *
 * @param now     Current time in ms.
 * @param target  Point in time to compare with.
 * @return true if now is at or after target.
 */
static bool reached(uint32_t now, uint32_t target)
{

    return (int32_t)(now - target) >= 0;
}

/**
 * @brief Small random number generator (xorshift32) for the thinking time.
 *
 * @param m  Match that holds the generator state.
 * @return Next random number.
 */
static uint32_t nextRandom(Match& m)
{

    m.rng ^= m.rng << 13;
    m.rng ^= m.rng >> 17;
    m.rng ^= m.rng << 5;

    return m.rng;
}

/**
 * @brief Checks that no stone lies on the board.
 *
 * @param in  Current inputs.
 * @return true if all sensors report "free".
 */
static bool noStones(const MatchInput& in)
{

    for(uint8_t i = 0; i < 9; i++)
    {
        if(in.occupied[i])
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks that the sensors show exactly the expected stones.
 *
 * Only the player's stones are physical, so a cell must be occupied
 * if and only if the board holds an X there.
 *
 * @param m   Match with the expected board.
 * @param in  Current inputs.
 * @return true if sensors and board agree on every cell.
 */
static bool matchesBoard(const Match& m, const MatchInput& in)
{

    for(uint8_t i = 0; i < 9; i++)
    {
        if(in.occupied[i] != (m.board[i] == CELL_PLAYER))
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Empties the board and removes the win line and the pending AI move.
 *
 * @param m  Match to reset.
 */
static void clearBoard(Match& m)
{

    for(uint8_t i = 0; i < 9; i++)
    {
        m.board[i] = 0;
    }

    m.hasWinLine  = false;
    m.pendingMove = -1;
}

/**
 * @brief Goes to READY: board cleared, dwell time started.
 *
 * @param m   Match to change.
 * @param in  Current inputs (time and sensors).
 */
static void enterReady(Match& m, const MatchInput& in)
{

    clearBoard(m);

    m.state   = MatchState::READY;
    m.message = noStones(in) ? MatchMessage::READY : MatchMessage::REMOVE_STONES;
    m.timer   = in.nowMs + READY_DWELL_MS;
}

/**
 * @brief Goes to FAULT with the given message.
 *
 * @param m    Match to change.
 * @param msg  Reason shown on the LCD.
 */
static void enterFault(Match& m, MatchMessage msg)
{

    m.state   = MatchState::FAULT;
    m.message = msg;
}

/**
 * @brief Ends the game, updates the score and the starting player.
 *
 * @param m             Match to change.
 * @param winnerValue   1 = player won, 2 = AI won, 0 = draw.
 */
static void finishGame(Match& m, uint8_t winnerValue)
{

    m.state = MatchState::GAME_OVER;

    if(winnerValue == CELL_PLAYER)
    {
        m.scorePlayer++;
        m.message = MatchMessage::PLAYER_WINS;
    }
    else if(winnerValue == CELL_AI)
    {
        m.scoreAi++;
        m.message = MatchMessage::AI_WINS;
    }
    else
    {
        m.scoreDraw++;
        m.message = MatchMessage::DRAW;
    }

    m.hasWinLine       = winLine(m.board, m.winLineCells);
    m.playerStartsNext = !m.playerStartsNext;
}

/**
 * @brief Checks the board after a move and finishes the game if it is over.
 *
 * @param m  Match to check.
 * @return true if the game has ended.
 */
static bool checkEnd(Match& m)
{

    uint8_t w = winner(m.board);

    if(w != 0)
    {
        finishGame(m, w);
        return true;
    }

    if(isFull(m.board))
    {
        finishGame(m, 0);
        return true;
    }

    return false;
}

/**
 * @brief Lets the AI choose its move and starts the thinking pause.
 *
 * The move is chosen right away (the LEDs can show it), but it is only
 * placed on the board when the pause is over.
 *
 * @param m   Match to change.
 * @param in  Current inputs (time).
 */
static void startAiThinking(Match& m, const MatchInput& in)
{

    int move = waehleZug(m.board, m.level);

    if(move < 0)   // cannot happen, the board is not full here
    {
        finishGame(m, 0);
        return;
    }

    m.pendingMove = (int8_t)move;
    m.timer       = in.nowMs + THINK_MIN_MS + nextRandom(m) % (THINK_MAX_MS - THINK_MIN_MS + 1);
    m.state       = MatchState::AI_THINKING;
    m.message     = MatchMessage::AI_THINKING;
}

/**
 * @brief Starts a new game: the player or the AI begins, alternating.
 *
 * @param m   Match to change.
 * @param in  Current inputs (time).
 */
static void startGame(Match& m, const MatchInput& in)
{

    clearBoard(m);

    if(m.playerStartsNext)
    {
        m.state   = MatchState::PLAYER_TURN;
        m.message = MatchMessage::YOUR_TURN;
    }
    else
    {
        startAiThinking(m, in);
    }
}

/**
 * @brief Switches to the next difficulty level (wraps around).
 *
 * @param m  Match to change.
 */
static void nextLevel(Match& m)
{

    m.level = (uint8_t)((m.level + 1) % LEVEL_COUNT);
}

/**
 * @brief State READY: waits for an empty board, then starts after the dwell time.
 *
 * Every press of the level button restarts the dwell time. The new-game
 * button starts at once. While stones lie on the board nothing starts.
 *
 * @param m   Match to change.
 * @param in  Current inputs.
 */
static void updateReady(Match& m, const MatchInput& in)
{

    if(in.buttonLevel)
    {
        nextLevel(m);
        m.timer = in.nowMs + READY_DWELL_MS;
    }

    if(!noStones(in))
    {
        m.message = MatchMessage::REMOVE_STONES;
        m.timer   = in.nowMs + READY_DWELL_MS;
        return;
    }

    m.message = MatchMessage::READY;

    if(in.buttonNewGame || reached(in.nowMs, m.timer))
    {
        startGame(m, in);
    }
}

/**
 * @brief State PLAYER_TURN: compares the sensors with the expected board.
 *
 * Exactly one new stone on an empty cell is a valid move. A missing stone,
 * a stone on a cell of the AI or two new stones lead to FAULT.
 *
 * @param m   Match to change.
 * @param in  Current inputs.
 */
static void updatePlayerTurn(Match& m, const MatchInput& in)
{

    uint8_t newCount = 0;
    uint8_t newCell  = 0;
    bool    onAiCell = false;
    bool    missing  = false;

    for(uint8_t i = 0; i < 9; i++)
    {
        if(in.occupied[i])
        {
            if(m.board[i] == 0)
            {
                newCount++;
                newCell = i;
            }
            else if(m.board[i] == CELL_AI)
            {
                onAiCell = true;
            }
        }
        else if(m.board[i] == CELL_PLAYER)
        {
            missing = true;
        }
    }

    if(missing)
    {
        enterFault(m, MatchMessage::STONE_MISSING);
        return;
    }

    if(onAiCell)
    {
        enterFault(m, MatchMessage::CELL_TAKEN);
        return;
    }

    if(newCount > 1)
    {
        enterFault(m, MatchMessage::ONE_STONE_ONLY);
        return;
    }

    if(newCount == 1)
    {
        m.board[newCell] = CELL_PLAYER;

        if(!checkEnd(m))
        {
            startAiThinking(m, in);
        }
    }
}

/**
 * @brief State AI_THINKING: sensors are ignored, the AI moves when the pause is over.
 *
 * @param m   Match to change.
 * @param in  Current inputs (time).
 */
static void updateAiThinking(Match& m, const MatchInput& in)
{

    if(!reached(in.nowMs, m.timer))
    {
        return;
    }

    m.board[m.pendingMove] = CELL_AI;
    m.pendingMove          = -1;

    if(!checkEnd(m))
    {
        m.state   = MatchState::PLAYER_TURN;
        m.message = MatchMessage::YOUR_TURN;
    }
}

/**
 * @brief State FAULT: waits until the sensors show exactly the expected board.
 *
 * @param m   Match to change.
 * @param in  Current inputs.
 */
static void updateFault(Match& m, const MatchInput& in)
{

    if(matchesBoard(m, in))
    {
        m.state   = MatchState::PLAYER_TURN;
        m.message = MatchMessage::YOUR_TURN;
    }
}

/**
 * @brief State GAME_OVER: the level can be changed, waits until all stones are removed.
 *
 * @param m   Match to change.
 * @param in  Current inputs.
 */
static void updateGameOver(Match& m, const MatchInput& in)
{

    if(in.buttonLevel)
    {
        nextLevel(m);
    }

    if(noStones(in))
    {
        enterReady(m, in);
    }
}

void matchInit(Match& m, uint32_t seed, uint32_t nowMs)
{

    clearBoard(m);

    m.state            = MatchState::READY;
    m.message          = MatchMessage::READY;
    m.level            = STUFE_MITTEL;
    m.scorePlayer      = 0;
    m.scoreAi          = 0;
    m.scoreDraw        = 0;
    m.playerStartsNext = true;
    m.winLineCells[0]  = 0;
    m.winLineCells[1]  = 0;
    m.winLineCells[2]  = 0;
    m.timer            = nowMs + READY_DWELL_MS;
    m.rng              = (seed == 0) ? 2463534242u : seed;   // xorshift must not start with 0
}

void matchUpdate(Match& m, const MatchInput& in)
{

    // The new-game button aborts a running game (it does not count in the score).
    if(in.buttonNewGame && m.state != MatchState::READY)
    {
        enterReady(m, in);
        return;
    }

    switch(m.state)
    {
        case MatchState::READY:
            updateReady(m, in);
            break;

        case MatchState::PLAYER_TURN:
            updatePlayerTurn(m, in);
            break;

        case MatchState::AI_THINKING:
            updateAiThinking(m, in);
            break;

        case MatchState::GAME_OVER:
            updateGameOver(m, in);
            break;

        case MatchState::FAULT:
            updateFault(m, in);
            break;
    }
}