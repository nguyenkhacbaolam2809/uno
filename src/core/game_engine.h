#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "card.h"
#include "deck.h"
#include "player.h"
#include "config.h"
#include "rules.h"
#include <string>
#include <vector>
#include <memory>

class IBotStrategy;

enum GamePhase
{
    PHASE_DEAL,
    PHASE_PLAY,
    PHASE_DRAW,
    PHASE_JUMP_IN,
    PHASE_GAME_OVER
};

enum JumpInState
{
    JUMP_NONE,
    JUMP_AVAILABLE,
    JUMP_TAKEN
};

enum BotActionType
{
    BOT_DRAW = -1,
    BOT_PLAY_CARD = 0,
    BOT_STACK_CARD = 1,
    BOT_JUMP_IN = 2
};

struct BotActionResult
{
    int action;
    int cardIdx;
    COLOR chosenColor;
};

struct GameState
{
    int turn;
    int direction;
    GamePhase phase;
    bool forceDraw;
    int drawStack;
    bool vietRules;
    card currentCard;
    int winner;
    int playerCount;
    JumpInState jumpState;
    int jumperId;
};

class GameEngine
{
public:
    GameEngine(const GameConfig & cfg, bool vietRules);
    ~GameEngine();

    void init(int numPlayers);
    int addPlayer(const std::string & name, PlayerType type, int difficulty);
    void start();
    void reset();

    GameState getState() const;
    const GameConfig & getConfig() const;
    bool isGameOver() const noexcept;

    int getCurrentTurn() const noexcept;
    int getDirection() const noexcept;
    const card & getCurrentCard() const noexcept;
    int getWinner() const noexcept;
    int getDrawStack() const noexcept;
    bool isForceDraw() const noexcept;
    GamePhase getPhase() const noexcept;

    player * getPlayer(int idx) noexcept;
    const player * getPlayer(int idx) const noexcept;
    int getPlayerCount() const noexcept;

    bool validatePlay(int playerIdx, int cardIdx) const;
    bool playCard(int playerIdx, int cardIdx, const std::string & chosenColor);
    void drawCard(int playerIdx);
    bool jumpIn(int playerIdx, int cardIdx);
    void callUno(int playerIdx);
    void catchUno(int /*callerIdx*/, int targetIdx);

    void nextTurn();
    void reshuffleDiscard();

    BotActionResult executeBotTurn(int playerIdx);
    BotActionResult executeBotJumpIn(int playerIdx);

private:
    GameConfig config;
    bool vietRules;

    deck mainDeck;
    deck discardPile;
    std::vector<player> players;
    std::vector<std::unique_ptr<IBotStrategy>> botStrategies;
    int playerCount;

    GameState state;

    void initDecks();
    void dealCards();
    void chooseRandomStarter();
    void applyActionCard(const card & c);
};

#endif
