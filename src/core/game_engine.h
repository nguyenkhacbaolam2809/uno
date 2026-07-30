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

enum class GamePhase
{
    Deal,
    Play,
    Draw,
    JumpIn,
    GameOver
};

enum class JumpInState
{
    None,
    Available,
    Taken
};

enum class BotAction
{
    Draw = -1,
    PlayCard = 0,
    StackCard = 1,
    JumpIn = 2
};

struct BotActionResult
{
    BotAction action;
    int cardIdx;
    CardColor chosenColor;
};

struct GameState
{
    int turn;
    int direction;
    GamePhase phase;
    bool forceDraw;
    int drawStack;
    bool vietRules;
    Card currentCard;
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

    [[nodiscard]] GameState getState() const;
    [[nodiscard]] const GameConfig & getConfig() const;
    [[nodiscard]] bool isGameOver() const noexcept;

    [[nodiscard]] int getCurrentTurn() const noexcept;
    [[nodiscard]] int getDirection() const noexcept;
    [[nodiscard]] const Card & getCurrentCard() const noexcept;
    [[nodiscard]] int getWinner() const noexcept;
    [[nodiscard]] int getDrawStack() const noexcept;
    [[nodiscard]] bool isForceDraw() const noexcept;
    [[nodiscard]] GamePhase getPhase() const noexcept;

    Player * getPlayer(int idx) noexcept;
    const Player * getPlayer(int idx) const noexcept;
    [[nodiscard]] int getPlayerCount() const noexcept;

    bool validatePlay(int playerIdx, int cardIdx) const;
    bool playCard(int playerIdx, int cardIdx, CardColor chosenColor);
    void drawCard(int playerIdx);
    bool jumpIn(int playerIdx, int cardIdx);
    void callUno(int playerIdx);
    void catchUno(int callerIdx, int targetIdx);

    void nextTurn();
    void reshuffleDiscard();

    BotActionResult executeBotTurn(int playerIdx);
    BotActionResult executeBotJumpIn(int playerIdx);

private:
    GameConfig config;

    Deck mainDeck;
    Deck discardPile;
    std::vector<Player> players;
    std::vector<std::unique_ptr<IBotStrategy>> botStrategies;
    int playerCount;

    GameState state;
    std::vector<bool> m_unoCalled;

    void initDecks();
    void dealCards();
    void chooseRandomStarter();
    void applyActionCard(const Card & c);
};

#endif
