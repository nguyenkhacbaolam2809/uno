#include "game_engine.h"
#include "bot_factory.h"
#include "rules.h"
#include "rng.h"

GameEngine::GameEngine(const GameConfig & cfg, bool viet)
    : config(cfg), vietRules(viet)
{
    state.turn = 0;
    state.direction = 1;
    state.phase = GamePhase::Deal;
    state.forceDraw = false;
    state.drawStack = 0;
    state.vietRules = vietRules;
    state.winner = -1;
    state.playerCount = 0;
    state.jumpState = JumpInState::None;
    state.jumperId = -1;
}

GameEngine::~GameEngine() = default;

void GameEngine::init(int numPlayers)
{
    botStrategies.clear();
    players.assign(numPlayers, Player());
    botStrategies.resize(numPlayers);

    for (int i = 0; i < numPlayers; i++)
        players[i].setName("");

    playerCount = numPlayers;
    state.playerCount = numPlayers;
    m_unoCalled.assign(numPlayers, false);
}

int GameEngine::addPlayer(const std::string & name, PlayerType type, int difficulty)
{
    for (int i = 0; i < playerCount; i++)
    {
        if (players[i].getName().empty() && botStrategies[i] == nullptr)
        {
            players[i] = Player(name, type, static_cast<BotDifficulty>(difficulty));
            if (type == PlayerType::Bot)
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
        Card c = mainDeck.draw();
        if (c.color != CardColor::Wild)
        {
            state.currentCard = c;
            state.turn = randomInt(0, playerCount - 1);
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
    state.phase = GamePhase::Deal;
    initDecks();
    dealCards();
    chooseRandomStarter();
    state.phase = GamePhase::Play;

    if (isActionCard(state.currentCard))
        applyActionCard(state.currentCard);

    state.winner = -1;
    state.jumpState = JumpInState::None;
    state.jumperId = -1;
}

void GameEngine::reset()
{
    botStrategies.clear();

    for (int i = 0; i < playerCount; i++)
    {
        while (players[i].get_size() > 0)
            players[i].hand_remove(0);
    }
    mainDeck = Deck();
    discardPile = Deck();

    state.turn = 0;
    state.direction = 1;
    state.phase = GamePhase::Deal;
    state.forceDraw = false;
    state.drawStack = 0;
    state.winner = -1;
    state.jumpState = JumpInState::None;
    state.jumperId = -1;
}

GameState GameEngine::getState() const { return state; }
const GameConfig & GameEngine::getConfig() const { return config; }
bool GameEngine::isGameOver() const noexcept { return state.winner >= 0 || state.phase == GamePhase::GameOver; }
int GameEngine::getCurrentTurn() const noexcept { return state.turn; }
int GameEngine::getDirection() const noexcept { return state.direction; }
const Card & GameEngine::getCurrentCard() const noexcept { return state.currentCard; }
int GameEngine::getWinner() const noexcept { return state.winner; }
int GameEngine::getDrawStack() const noexcept { return state.drawStack; }
bool GameEngine::isForceDraw() const noexcept { return state.forceDraw; }
GamePhase GameEngine::getPhase() const noexcept { return state.phase; }
Player * GameEngine::getPlayer(int idx) noexcept { return &players[idx]; }
const Player * GameEngine::getPlayer(int idx) const noexcept { return &players[idx]; }
int GameEngine::getPlayerCount() const noexcept { return playerCount; }

bool GameEngine::validatePlay(int playerIdx, int cardIdx) const
{
    if (playerIdx < 0 || playerIdx >= playerCount) return false;
    if (cardIdx < 0 || cardIdx >= players[playerIdx].get_size()) return false;

    Card chosen = players[playerIdx].peek(cardIdx);
    if (!::canPlayCard(chosen, state.currentCard)) return false;
    if (chosen.number == CARD_WILD_DRAW_FOUR && !::canPlayWildDrawFour(chosen, state.currentCard, players[playerIdx]))
        return false;
    return true;
}

bool GameEngine::playCard(int playerIdx, int cardIdx, CardColor chosenColor)
{
    if (playerIdx < 0 || playerIdx >= playerCount) return false;
    if (cardIdx < 0 || cardIdx >= players[playerIdx].get_size()) return false;

    Card chosen = players[playerIdx].peek(cardIdx);
    if (!::canPlayCard(chosen, state.currentCard)) return false;
    if (chosen.number == CARD_WILD_DRAW_FOUR && !::canPlayWildDrawFour(chosen, state.currentCard, players[playerIdx]))
        return false;

    if (vietRules && players[playerIdx].get_size() == 1 && !::isLegalLastCard(chosen))
        return false;

    players[playerIdx].hand_remove(cardIdx);

    if (chosen.color == CardColor::Wild)
        chosen.color = chosenColor;

    state.currentCard = chosen;
    discardPile.add_card(chosen);

    if (isStackCard(chosen))
    {
        state.drawStack += getStackValue(chosen);
        state.forceDraw = true;
    }

    state.jumpState = JumpInState::Available;
    state.jumperId = -1;

    if (players[playerIdx].get_size() == 0)
    {
        state.winner = playerIdx;
        state.phase = GamePhase::GameOver;
        return true;
    }

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

    Card drawn = mainDeck.draw();
    players[playerIdx].hand_add(drawn);
}

void GameEngine::reshuffleDiscard()
{
    int n = discardPile.get_size();
    if (n <= 1)
        return;

    std::vector<Card> tmp;
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
    if (state.jumpState != JumpInState::Available) return false;

    Card chosen = players[playerIdx].peek(cardIdx);
    if (!::canJumpIn(chosen, state.currentCard)) return false;

    players[playerIdx].hand_remove(cardIdx);
    state.currentCard = chosen;
    discardPile.add_card(chosen);
    state.jumpState = JumpInState::Taken;
    state.jumperId = playerIdx;

    if (isStackCard(chosen))
    {
        state.drawStack += getStackValue(chosen);
        state.forceDraw = true;
    }

    if (players[playerIdx].get_size() == 0)
    {
        state.winner = playerIdx;
        state.phase = GamePhase::GameOver;
        return true;
    }

    return true;
}

void GameEngine::callUno(int playerIdx)
{
    if (playerIdx < 0 || playerIdx >= playerCount) return;
    m_unoCalled[playerIdx] = true;
}

void GameEngine::catchUno(int callerIdx, int targetIdx)
{
    if (!vietRules) return;
    if (targetIdx < 0 || targetIdx >= playerCount) return;
    if (players[targetIdx].get_size() != 1) return;

    int penaltyTarget = targetIdx;
    if (m_unoCalled[targetIdx])
        penaltyTarget = callerIdx;

    for (int i = 0; i < 2; i++)
    {
        if (mainDeck.get_size() == 0)
            reshuffleDiscard();
        players[penaltyTarget].hand_add(mainDeck.draw());
    }
}

void GameEngine::applyActionCard(const Card & c)
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
    else if (isStackCard(c))
    {
        state.turn += state.direction;
    }
}

void GameEngine::nextTurn()
{
    if (state.jumpState == JumpInState::Taken && state.jumperId >= 0)
    {
        state.turn = state.jumperId;
        state.jumperId = -1;
        state.jumpState = JumpInState::None;
        return;
    }

    // Clear UNO flag for outgoing player
    if (state.turn >= 0 && state.turn < playerCount)
        m_unoCalled[state.turn] = false;

    state.jumpState = JumpInState::None;
    state.turn = (state.turn + state.direction) % playerCount;
    if (state.turn < 0)
        state.turn += playerCount;
}

BotActionResult GameEngine::executeBotTurn(int playerIdx)
{
    BotActionResult result;
    result.action = BotAction::Draw;
    result.cardIdx = -1;
    result.chosenColor = CardColor::Wild;

    if (playerIdx < 0 || playerIdx >= playerCount)
        return result;

    IBotStrategy * strategy = botStrategies[playerIdx].get();
    if (!strategy)
        return result;

    Player * self = &players[playerIdx];

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
            Card c = self->peek(i);
            if (::isStackCard(c) && ::canPlayCard(c, state.currentCard))
            {
                stackDefIdx = i;
                break;
            }
        }

        if (stackDefIdx >= 0)
        {
            CardColor col = CardColor::Wild;
            Card chosen = self->peek(stackDefIdx);
            if (chosen.color == CardColor::Wild)
                col = strategy->pickColor(self);

            result.action = BotAction::StackCard;
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

    CardColor col = CardColor::Wild;
    Card chosen = self->peek(cardIdx);
    if (chosen.color == CardColor::Wild)
        col = strategy->pickColor(self);

    result.action = BotAction::PlayCard;
    result.cardIdx = cardIdx;
    result.chosenColor = col;
    return result;
}

BotActionResult GameEngine::executeBotJumpIn(int playerIdx)
{
    BotActionResult result;
    result.action = BotAction::Draw;
    result.cardIdx = -1;
    result.chosenColor = CardColor::Wild;

    if (playerIdx < 0 || playerIdx >= playerCount) return result;
    if (state.jumpState != JumpInState::Available) return result;

    IBotStrategy * strategy = botStrategies[playerIdx].get();
    if (!strategy) return result;

    Player * self = &players[playerIdx];
    if (!strategy->shouldJumpIn(self, state.currentCard))
        return result;

    for (int i = 0; i < self->get_size(); i++)
    {
        Card c = self->peek(i);
        if (::canJumpIn(c, state.currentCard))
        {
            CardColor col = CardColor::Wild;
            if (c.color == CardColor::Wild)
                col = strategy->pickColor(self);

            result.action = BotAction::JumpIn;
            result.cardIdx = i;
            result.chosenColor = col;
            return result;
        }
    }

    return result;
}
