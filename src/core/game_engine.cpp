#include "game_engine.h"
#include "bot_factory.h"
#include "rules.h"
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iostream>

GameEngine::GameEngine(const GameConfig & cfg, bool vietRules)
    : config(cfg), vietRules(vietRules)
{
    state.turn = 0;
    state.direction = 1;
    state.phase = PHASE_DEAL;
    state.forceDraw = false;
    state.drawStack = 0;
    state.vietRules = vietRules;
    state.winner = -1;
    state.playerCount = 0;
    state.jumpState = JUMP_NONE;
    state.jumperId = -1;
}

GameEngine::~GameEngine()
{
    destroyStrategies();
}

void GameEngine::destroyStrategies()
{
    for (std::size_t i = 0; i < botStrategies.size(); i++)
    {
        if (botStrategies[i])
            delete botStrategies[i];
    }
    botStrategies.clear();
}

void GameEngine::init(int numPlayers)
{
    destroyStrategies();

    players.assign(numPlayers, player());
    botStrategies.assign(numPlayers, nullptr);

    for (int i = 0; i < numPlayers; i++)
        players[i].setName("");

    playerCount = numPlayers;
    state.playerCount = numPlayers;
}

int GameEngine::addPlayer(const std::string & name, PlayerType type, int difficulty)
{
    for (int i = 0; i < playerCount; i++)
    {
        if (players[i].getName().empty() && botStrategies[i] == nullptr)
        {
            players[i] = player(name, type, static_cast<BotDifficulty>(difficulty));
            if (type == BOT)
                botStrategies[i] = createBotStrategy(static_cast<BotDifficulty>(difficulty));
            return i;
        }
    }
    return -1;
}

void GameEngine::initDecks()
{
    mainDeck.create();
    mainDeck.quick_shuffle();
}

void GameEngine::dealCards()
{
    for (int i = 0; i < playerCount; i++)
        for (int k = 0; k < HAND_SIZE; k++)
            players[i].hand_add(mainDeck.draw());
}

void GameEngine::chooseRandomStarter()
{
    int maxAttempts = playerCount * 10;
    while (maxAttempts > 0)
    {
        card c = mainDeck.draw();
        if (c.color != wild)
        {
            state.currentCard = c;
            state.turn = rand() % playerCount;
            return;
        }
        discardPile.add_card(c);
        maxAttempts--;
    }

    state.currentCard = mainDeck.draw();
    state.turn = 0;
}

void GameEngine::start()
{
    state.phase = PHASE_DEAL;
    initDecks();
    dealCards();
    chooseRandomStarter();
    state.phase = PHASE_PLAY;

    if (isActionCard(state.currentCard))
        applyActionCard(state.currentCard);

    state.winner = -1;
    state.jumpState = JUMP_NONE;
    state.jumperId = -1;
}

void GameEngine::reset()
{
    destroyStrategies();

    for (int i = 0; i < playerCount; i++)
    {
        while (players[i].get_size() > 0)
            players[i].hand_remove(0);
    }
    mainDeck = deck();
    discardPile = deck();

    state.turn = 0;
    state.direction = 1;
    state.phase = PHASE_DEAL;
    state.forceDraw = false;
    state.drawStack = 0;
    state.winner = -1;
    state.jumpState = JUMP_NONE;
    state.jumperId = -1;
}

GameState GameEngine::getState() const { return state; }
const GameConfig & GameEngine::getConfig() const { return config; }
bool GameEngine::isGameOver() const { return state.winner >= 0 || state.phase == PHASE_GAME_OVER; }
int GameEngine::getCurrentTurn() const { return state.turn; }
int GameEngine::getDirection() const { return state.direction; }
const card & GameEngine::getCurrentCard() const { return state.currentCard; }
int GameEngine::getWinner() const { return state.winner; }
int GameEngine::getDrawStack() const { return state.drawStack; }
bool GameEngine::isForceDraw() const { return state.forceDraw; }
GamePhase GameEngine::getPhase() const { return state.phase; }
player * GameEngine::getPlayer(int idx) { return &players[idx]; }
const player * GameEngine::getPlayer(int idx) const { return &players[idx]; }
int GameEngine::getPlayerCount() const { return playerCount; }

bool GameEngine::validatePlay(int playerIdx, int cardIdx) const
{
    if (playerIdx < 0 || playerIdx >= playerCount) return false;
    if (cardIdx < 0 || cardIdx >= players[playerIdx].get_size()) return false;

    card chosen = players[playerIdx].peek(cardIdx);
    return ::canPlayCard(chosen, state.currentCard);
}

bool GameEngine::playCard(int playerIdx, int cardIdx, const std::string & chosenColor)
{
    if (playerIdx < 0 || playerIdx >= playerCount) return false;
    if (cardIdx < 0 || cardIdx >= players[playerIdx].get_size()) return false;

    card chosen = players[playerIdx].peek(cardIdx);
    if (!::canPlayCard(chosen, state.currentCard)) return false;

    if (vietRules && players[playerIdx].get_size() == 1 && !::isLegalLastCard(chosen))
        return false;

    players[playerIdx].hand_remove(cardIdx);

    if (chosen.color == wild)
    {
        COLOR col = wild;
        if (chosenColor == "do" || chosenColor == "red")           col = red;
        else if (chosenColor == "xanh la" || chosenColor == "green")  col = green;
        else if (chosenColor == "xanh duong" || chosenColor == "blue") col = blue;
        else if (chosenColor == "vang" || chosenColor == "yellow")    col = yellow;
        if (col != wild)
            chosen.color = col;
    }

    state.currentCard = chosen;
    discardPile.add_card(chosen);

    if (isStackCard(chosen))
    {
        state.drawStack += getStackValue(chosen);
        state.forceDraw = true;
    }

    state.jumpState = JUMP_AVAILABLE;
    state.jumperId = -1;

    if (players[playerIdx].get_size() == 0)
    {
        state.winner = playerIdx;
        state.phase = PHASE_GAME_OVER;
        return true;
    }

    if (isActionCard(chosen) && !isStackCard(chosen))
        applyActionCard(chosen);

    return true;
}

void GameEngine::drawCard(int playerIdx)
{
    if (playerIdx < 0 || playerIdx >= playerCount) return;

    if (state.forceDraw)
    {
        int totalDraw = state.drawStack;
        state.drawStack = 0;
        state.forceDraw = false;

        for (int i = 0; i < totalDraw; i++)
        {
            if (mainDeck.get_size() == 0)
                reshuffleDiscard();
            players[playerIdx].hand_add(mainDeck.draw());
        }
        return;
    }

    if (mainDeck.get_size() == 0)
        reshuffleDiscard();

    card drawn = mainDeck.draw();
    players[playerIdx].hand_add(drawn);
}

void GameEngine::reshuffleDiscard()
{
    int n = discardPile.get_size();
    if (n <= 1)
    {
        if (mainDeck.get_size() == 0 && n > 0)
        {
            card c = discardPile.draw();
            state.currentCard = c;
            discardPile.add_card(c);
        }
        return;
    }

    std::vector<card> tmp;
    tmp.reserve(n);
    for (int i = 0; i < n; i++)
        tmp.push_back(discardPile.draw());

    for (int i = 0; i < n - 1; i++)
        mainDeck.add_card(tmp[i]);

    state.currentCard = tmp[n - 1];
    discardPile.add_card(state.currentCard);
    mainDeck.quick_shuffle();
}

bool GameEngine::jumpIn(int playerIdx, int cardIdx)
{
    if (!vietRules) return false;
    if (playerIdx < 0 || playerIdx >= playerCount) return false;
    if (cardIdx < 0 || cardIdx >= players[playerIdx].get_size()) return false;
    if (state.jumpState != JUMP_AVAILABLE) return false;

    card chosen = players[playerIdx].peek(cardIdx);
    if (!::canJumpIn(chosen, state.currentCard)) return false;

    players[playerIdx].hand_remove(cardIdx);
    state.currentCard = chosen;
    discardPile.add_card(chosen);
    state.jumpState = JUMP_TAKEN;
    state.jumperId = playerIdx;

    if (isStackCard(chosen))
    {
        state.drawStack += getStackValue(chosen);
        state.forceDraw = true;
    }

    if (players[playerIdx].get_size() == 0)
    {
        state.winner = playerIdx;
        state.phase = PHASE_GAME_OVER;
        return true;
    }

    return true;
}

void GameEngine::callUno(int playerIdx)
{
    if (playerIdx < 0 || playerIdx >= playerCount) return;
    if (players[playerIdx].get_size() == 1)
        state.jumpState = JUMP_AVAILABLE;
}

void GameEngine::catchUno(int callerIdx, int targetIdx)
{
    if (!vietRules) return;
    if (targetIdx < 0 || targetIdx >= playerCount) return;
    if (players[targetIdx].get_size() != 1) return;

    for (int i = 0; i < 2; i++)
    {
        if (mainDeck.get_size() == 0)
            reshuffleDiscard();
        players[targetIdx].hand_add(mainDeck.draw());
    }
}

void GameEngine::applyActionCard(const card & c)
{
    if (c.number == CARD_SKIP)
    {
        state.turn += state.direction;
    }
    else if (c.number == CARD_REVERSE)
    {
        if (playerCount == 2)
            state.turn += state.direction;
        else
            state.direction = -state.direction;
    }
}

void GameEngine::nextTurn()
{
    if (state.jumpState == JUMP_TAKEN && state.jumperId >= 0)
    {
        state.turn = state.jumperId;
        state.jumperId = -1;
        state.jumpState = JUMP_NONE;
        return;
    }

    state.jumpState = JUMP_NONE;
    state.turn = (state.turn + state.direction) % playerCount;
    if (state.turn < 0)
        state.turn += playerCount;
}

BotActionResult GameEngine::executeBotTurn(int playerIdx)
{
    BotActionResult result;
    result.action = BOT_DRAW;
    result.cardIdx = -1;
    result.chosenColor = wild;

    if (playerIdx < 0 || playerIdx >= playerCount)
        return result;

    IBotStrategy * strategy = botStrategies[playerIdx];
    if (!strategy)
        return result;

    player * self = &players[playerIdx];

    int oppSizes[5] = {0};
    int oppCount = 0;
    for (int i = 0; i < playerCount; i++)
    {
        if (i != playerIdx)
        {
            oppSizes[oppCount] = players[i].get_size();
            oppCount++;
        }
    }

    if (state.forceDraw && state.drawStack > 0)
    {
        int stackDefIdx = -1;
        for (int i = 0; i < self->get_size(); i++)
        {
            card c = self->peek(i);
            if (::isStackCard(c) && ::canPlayCard(c, state.currentCard))
            {
                stackDefIdx = i;
                break;
            }
        }

        if (stackDefIdx >= 0)
        {
            COLOR col = wild;
            card chosen = self->peek(stackDefIdx);
            if (chosen.color == wild)
                col = strategy->pickColor(self);

            result.action = BOT_STACK_CARD;
            result.cardIdx = stackDefIdx;
            result.chosenColor = col;
            return result;
        }

        return result;
    }

    int cardIdx = strategy->pickCard(self, state.currentCard,
                                     state.direction, playerIdx,
                                     oppSizes, oppCount,
                                     state.drawStack);

    if (cardIdx < 0)
        return result;

    COLOR col = wild;
    card chosen = self->peek(cardIdx);
    if (chosen.color == wild)
        col = strategy->pickColor(self);

    result.action = BOT_PLAY_CARD;
    result.cardIdx = cardIdx;
    result.chosenColor = col;
    return result;
}

BotActionResult GameEngine::executeBotJumpIn(int playerIdx)
{
    BotActionResult result;
    result.action = BOT_DRAW;
    result.cardIdx = -1;
    result.chosenColor = wild;

    if (playerIdx < 0 || playerIdx >= playerCount) return result;
    if (state.jumpState != JUMP_AVAILABLE) return result;

    IBotStrategy * strategy = botStrategies[playerIdx];
    if (!strategy) return result;

    player * self = &players[playerIdx];
    if (!strategy->shouldJumpIn(self, state.currentCard))
        return result;

    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (::canJumpIn(c, state.currentCard))
        {
            COLOR col = wild;
            if (c.color == wild)
                col = strategy->pickColor(self);

            result.action = BOT_JUMP_IN;
            result.cardIdx = i;
            result.chosenColor = col;
            return result;
        }
    }

    return result;
}
