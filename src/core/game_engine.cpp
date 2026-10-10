#include "game_engine.h"
#include "bot_factory.h"
#include "rules.h"
#include "rng.h"
#include <algorithm>

GameEngine::GameEngine(const GameConfig & cfg, bool viet)
    : config(cfg), playerCount(0)
{
    state.turn = 0;
    state.direction = 1;
    state.phase = GamePhase::Deal;
    state.forceDraw = false;
    state.drawStack = 0;
    state.vietRules = viet;
    state.winner = -1;
    state.playerCount = 0;
    state.jumpState = JumpInState::None;
    state.jumperId = -1;
    m_finished.assign(playerCount, false);
    m_finishOrder.clear();
}

GameEngine::~GameEngine() = default;

void GameEngine::init(int numPlayers)
{
    numPlayers = std::clamp(numPlayers, 0, MAX_PLAYERS);
    botStrategies.clear();
    players.assign(numPlayers, Player());
    botStrategies.resize(numPlayers);

    for (int i = 0; i < numPlayers; i++)
        players[i].setName("");

    playerCount = numPlayers;
    state.playerCount = numPlayers;
    m_unoCalled.assign(numPlayers, false);
    m_finished.assign(numPlayers, false);
    m_finishOrder.clear();
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
        if (c.color != CardColor::Wild && !isActionCard(c))
        {
            state.currentCard = c;
            discardPile.add_card(c);
            state.turn = randomInt(0, playerCount - 1);
            return;
        }
        discardPile.add_card(c);
        maxAttempts--;
    }

    state.currentCard = mainDeck.draw();
    discardPile.add_card(state.currentCard);
    state.turn = 0;
}

void GameEngine::start()
{
    if (playerCount <= 0)
        return;

    state.phase = GamePhase::Deal;
    state.direction = 1;
    state.forceDraw = false;
    state.drawStack = 0;
    state.winner = -1;
    state.jumpState = JumpInState::None;
    state.jumperId = -1;
    m_finished.assign(playerCount, false);
    m_finishOrder.clear();

    for (Player & player : players)
    {
        while (player.get_size() > 0)
            player.hand_remove(0);
    }
    discardPile = Deck();
    m_unoCalled.assign(playerCount, false);

    initDecks();
    dealCards();
    chooseRandomStarter();
    state.phase = GamePhase::Play;

    if (isActionCard(state.currentCard))
        applyActionCard(state.currentCard);

    // If the starting card is a stack card (Draw2/WD4),
    // the first player must draw penalty cards on their turn.
    if (isStackCard(state.currentCard))
    {
        state.drawStack = getStackValue(state.currentCard);
        state.forceDraw = true;
    }
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
bool GameEngine::isGameOver() const noexcept { return state.phase == GamePhase::GameOver; }
int GameEngine::getCurrentTurn() const noexcept { return state.turn; }
int GameEngine::getDirection() const noexcept { return state.direction; }
const Card & GameEngine::getCurrentCard() const noexcept { return state.currentCard; }
int GameEngine::getWinner() const noexcept { return state.winner; }
const std::vector<int> & GameEngine::getFinishOrder() const noexcept { return m_finishOrder; }
int GameEngine::getDrawStack() const noexcept { return state.drawStack; }
bool GameEngine::isForceDraw() const noexcept { return state.forceDraw; }
GamePhase GameEngine::getPhase() const noexcept { return state.phase; }
Player * GameEngine::getPlayer(int idx) noexcept
{
    if (idx < 0 || idx >= playerCount) return nullptr;
    return &players[idx];
}

const Player * GameEngine::getPlayer(int idx) const noexcept
{
    if (idx < 0 || idx >= playerCount) return nullptr;
    return &players[idx];
}
int GameEngine::getPlayerCount() const noexcept { return playerCount; }

bool GameEngine::validatePlay(int playerIdx, int cardIdx) const
{
    if (state.phase != GamePhase::Play || playerIdx != state.turn) return false;
    if (playerIdx < 0 || playerIdx >= playerCount) return false;
    if (cardIdx < 0 || cardIdx >= players[playerIdx].get_size()) return false;

    Card chosen = players[playerIdx].peek(cardIdx);
    if (!::canPlayCard(chosen, state.currentCard)) return false;
    if (state.forceDraw && !::isStackCard(chosen)) return false;
    if (chosen.number == CARD_WILD_DRAW_FOUR && !::canPlayWildDrawFour(chosen, state.currentCard, players[playerIdx]))
        return false;
    if (state.vietRules && players[playerIdx].get_size() == 1 && !::isLegalLastCard(chosen))
        return false;
    return true;
}

bool GameEngine::playCard(int playerIdx, int cardIdx, CardColor chosenColor)
{
    if (state.phase != GamePhase::Play || playerIdx != state.turn) return false;
    if (playerIdx < 0 || playerIdx >= playerCount) return false;
    if (cardIdx < 0 || cardIdx >= players[playerIdx].get_size()) return false;

    Card chosen = players[playerIdx].peek(cardIdx);
    if (!::canPlayCard(chosen, state.currentCard)) return false;
    if (state.forceDraw && !::isStackCard(chosen)) return false;
    if (chosen.color == CardColor::Wild &&
        (chosenColor < CardColor::Red || chosenColor > CardColor::Yellow))
        return false;
    if (chosen.number == CARD_WILD_DRAW_FOUR && !::canPlayWildDrawFour(chosen, state.currentCard, players[playerIdx]))
        return false;

    if (state.vietRules && players[playerIdx].get_size() == 1 && !::isLegalLastCard(chosen))
        return false;

    players[playerIdx].hand_remove(cardIdx);
    if (players[playerIdx].get_size() > 1)
        m_unoCalled[playerIdx] = false;

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
        recordFinish(playerIdx);
        return true;
    }

    applyActionCard(chosen);

    return true;
}

void GameEngine::drawCard(int playerIdx)
{
    if (playerIdx < 0 || playerIdx >= playerCount) return;
    m_unoCalled[playerIdx] = false;

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

    // tmp[0] is the top of discard (most recent = current card).
    // Keep it as currentCard, shuffle the rest into mainDeck.
    state.currentCard = tmp[0];
    for (int i = 1; i < n; i++)
        mainDeck.add_card(tmp[i]);

    discardPile.add_card(state.currentCard);
    mainDeck.quick_shuffle();
}

bool GameEngine::jumpIn(int playerIdx, int cardIdx)
{
    if (!state.vietRules) return false;
    if (playerIdx < 0 || playerIdx >= playerCount) return false;
    if (cardIdx < 0 || cardIdx >= players[playerIdx].get_size()) return false;
    if (state.jumpState != JumpInState::Available) return false;

    Card chosen = players[playerIdx].peek(cardIdx);
    if (!::canJumpIn(chosen, state.currentCard)) return false;
    if (players[playerIdx].get_size() == 1 && !::isLegalLastCard(chosen)) return false;

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
        recordFinish(playerIdx);
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
    if (!state.vietRules) return;
    if (callerIdx < 0 || callerIdx >= playerCount) return;
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
    m_unoCalled[penaltyTarget] = false;
}

void GameEngine::applyActionCard(const Card & c)
{
    if (c.number == CARD_SKIP)
    {
        state.turn = (state.turn + state.direction) % playerCount;
        if (state.turn < 0)
            state.turn += playerCount;
    }
    else if (c.number == CARD_REVERSE)
    {
        if (playerCount == 2)
        {
            state.turn = (state.turn + state.direction) % playerCount;
            if (state.turn < 0)
                state.turn += playerCount;
        }
        else
        {
            state.direction = -state.direction;
        }
    }
    // Note: stack cards (Draw2, WD4) do NOT advance the turn here.
    // The next player faces the forceDraw penalty on their turn,
    // and nextTurn() handles the single-step advance.
}

void GameEngine::nextTurn()
{
    if (playerCount <= 0) return;

    if (state.jumpState == JumpInState::Taken && state.jumperId >= 0)
    {
        // Clear UNO flag for outgoing player if not at 1 card
        if (state.turn >= 0 && state.turn < playerCount && players[state.turn].get_size() != 1)
            m_unoCalled[state.turn] = false;

        // Set base turn to the jumper
        state.turn = state.jumperId;
        state.jumperId = -1;
        state.jumpState = JumpInState::None;

        // Apply action card effects (Skip, Reverse) from jumper's position
        applyActionCard(state.currentCard);

        // Advance past the jumper (they already played in jumpIn())
        int next = state.turn;
        for (int i = 0; i < playerCount; i++)
        {
            next = (next + state.direction) % playerCount;
            if (next < 0) next += playerCount;
            if (!m_finished[next]) break;
        }
        state.turn = next;
        return;
    }

    // Clear UNO flag for outgoing player only if they don't have exactly 1 card left
    if (state.turn >= 0 && state.turn < playerCount && players[state.turn].get_size() != 1)
        m_unoCalled[state.turn] = false;

    state.jumpState = JumpInState::None;
    int next = state.turn;
    for (int i = 0; i < playerCount; i++)
    {
        next = (next + state.direction) % playerCount;
        if (next < 0) next += playerCount;
        if (!m_finished[next]) break;
    }
    state.turn = next;
}

void GameEngine::recordFinish(int playerIdx)
{
    if (playerIdx < 0 || playerIdx >= playerCount || m_finished[playerIdx])
        return;

    m_finished[playerIdx] = true;
    m_finishOrder.push_back(playerIdx);
    if (state.winner < 0)
        state.winner = playerIdx;

    if (countActivePlayers() <= 1)
        state.phase = GamePhase::GameOver;
}

int GameEngine::countActivePlayers() const noexcept
{
    int active = 0;
    for (bool finished : m_finished)
        if (!finished) ++active;
    return active;
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

    // Validate the strategy's choice (catches illegal WD4, viet-rules last card, etc.)
    if (cardIdx >= 0 && !validatePlay(playerIdx, cardIdx))
    {
        // Strategy picked an illegal card; find any legal alternative
        cardIdx = -1;
        for (int i = 0; i < self->get_size(); i++)
        {
            if (validatePlay(playerIdx, i))
            {
                cardIdx = i;
                break;
            }
        }
    }

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
