#include "card.h"
#include "deck.h"
#include "player.h"
#include "rules.h"
#include "game_engine.h"
#include "config.h"
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <string>

int testsPassed = 0;
int testsFailed = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { \
        std::cerr << "FAIL: " << name << " (" << #expr << ")" << std::endl; \
        testsFailed++; \
    } else { \
        testsPassed++; \
    } \
} while(0)

void testCard()
{
    Card a(5, CardColor::Red);
    Card b(5, CardColor::Green);
    Card c(7, CardColor::Red);
    Card wc(CARD_WILD, CardColor::Wild);
    Card d4(CARD_WILD_DRAW_FOUR, CardColor::Wild);

    TEST("same number diff color not equal", !(a == b));
    TEST("same color diff number not equal", !(a == c));
    TEST("wild and d4 not equal", !(wc == d4));
    TEST("identical cards equal", a == Card(5, CardColor::Red));

    // canPlayCard tests
    TEST("canPlayCard same number", canPlayCard(b, a));
    TEST("canPlayCard same color", canPlayCard(c, a));
    TEST("canPlayCard wild on anything", canPlayCard(wc, a));
    TEST("canPlayCard wild on wild", canPlayCard(wc, d4));
    TEST("canPlayCard incompatible", !canPlayCard(b, c));

    // isActionCard
    TEST("skip is action", isActionCard(Card(11, CardColor::Red)));
    TEST("reverse is action", isActionCard(Card(12, CardColor::Green)));
    TEST("draw2 is action", isActionCard(Card(10, CardColor::Blue)));
    TEST("wild is action", isActionCard(Card(13, CardColor::Wild)));
    TEST("wild draw4 is action", isActionCard(Card(14, CardColor::Wild)));
    TEST("number 5 is not action", !isActionCard(Card(5, CardColor::Red)));

    // isStackCard
    TEST("draw2 is stack", isStackCard(Card(10, CardColor::Red)));
    TEST("wild draw4 is stack", isStackCard(Card(14, CardColor::Wild)));
    TEST("skip is not stack", !isStackCard(Card(11, CardColor::Red)));
    TEST("number is not stack", !isStackCard(Card(5, CardColor::Green)));

    // getStackValue
    TEST("draw2 stack value 2", getStackValue(Card(10, CardColor::Blue)) == 2);
    TEST("wild draw4 stack value 4", getStackValue(Card(14, CardColor::Wild)) == 4);

    // isLegalLastCard (Vietnamese rules)
    TEST("number is legal last", isLegalLastCard(Card(3, CardColor::Red)));
    TEST("skip is legal last", isLegalLastCard(Card(11, CardColor::Green)));
    TEST("reverse is legal last", isLegalLastCard(Card(12, CardColor::Blue)));
    TEST("draw2 not legal last", !isLegalLastCard(Card(10, CardColor::Yellow)));
    TEST("wild not legal last", !isLegalLastCard(Card(13, CardColor::Wild)));
    TEST("wild draw4 not legal last", !isLegalLastCard(Card(14, CardColor::Wild)));

    // canJumpIn (Vietnamese rules) - requires exact same card (number AND color)
    TEST("jump in exact same card", canJumpIn(Card(3, CardColor::Red), Card(3, CardColor::Red)));
    TEST("jump in different color", !canJumpIn(Card(3, CardColor::Red), Card(3, CardColor::Green)));
    TEST("jump in different number", !canJumpIn(Card(5, CardColor::Red), Card(3, CardColor::Red)));
    TEST("jump in completely different", !canJumpIn(Card(5, CardColor::Red), Card(3, CardColor::Green)));
}

void testDeck()
{
    Deck mainDeck;
    TEST("deck initially empty", mainDeck.get_size() == 0);

    mainDeck.create();
    int initialSize = mainDeck.get_size();

    {
        int expected = 108;
        TEST("deck size 108 after create", initialSize == expected);
    }

    for (int i = 0; i < initialSize; i++)
        mainDeck.draw();
    TEST("deck empty after drawing all", mainDeck.get_size() == 0);

    Card emptyCard = mainDeck.draw();
    TEST("draw from empty returns wild", emptyCard.color == CardColor::Wild);

    Card c(5, CardColor::Red);
    mainDeck.add_card(c);
    TEST("deck has 1 card after add", mainDeck.get_size() == 1);

    Card drawn = mainDeck.draw();
    TEST("drawn card matches added", drawn.number == 5 && drawn.color == CardColor::Red);

    mainDeck.create();
    Deck copyDeck(mainDeck);
    TEST("copy deck same size", copyDeck.get_size() == mainDeck.get_size());

    Deck assignDeck;
    assignDeck = mainDeck;
    TEST("assign deck same size", assignDeck.get_size() == mainDeck.get_size());

    mainDeck.quick_shuffle();
    TEST("shuffle preserves size", mainDeck.get_size() == 108);
}

void testPlayer()
{
    Player p("TestPlayer", PlayerType::Human, BotDifficulty::Easy);
    TEST("player name", p.getName() == "TestPlayer");
    TEST("player type HUMAN", p.getType() == PlayerType::Human);
    TEST("player difficulty", p.getDifficulty() == BotDifficulty::Easy);
    TEST("player isBot false", !p.isBot());
    TEST("player starts empty", p.get_size() == 0);

    Card c1(3, CardColor::Red);
    Card c2(7, CardColor::Blue);
    Card c3(10, CardColor::Green);

    p.hand_add(c1);
    TEST("player has 1 card", p.get_size() == 1);
    TEST("peek matches added", p.peek(0).number == 3 && p.peek(0).color == CardColor::Red);

    p.hand_add(c2);
    p.hand_add(c3);
    TEST("player has 3 cards", p.get_size() == 3);

    Card invalid = p.peek(-1);
    TEST("peek -1 returns safe card", invalid.color == CardColor::Wild);
    invalid = p.peek(100);
    TEST("peek out of bounds safe", invalid.color == CardColor::Wild);

    Card removed = p.hand_remove(1);
    TEST("removed correct card", removed.number == 7 && removed.color == CardColor::Blue);
    TEST("player has 2 cards after remove", p.get_size() == 2);

    removed = p.hand_remove(-1);
    TEST("remove -1 returns safe card", removed.color == CardColor::Wild);
    removed = p.hand_remove(100);
    TEST("remove out of bounds safe", removed.color == CardColor::Wild);

    Player p2(p);
    TEST("copy constructor same size", p2.get_size() == p.get_size());

    Player p3;
    p3 = p;
    TEST("assignment same size", p3.get_size() == p.get_size());

    Player bot("Bot1", PlayerType::Bot, BotDifficulty::Hard);
    TEST("bot isBot true", bot.isBot());
    TEST("bot type BOT", bot.getType() == PlayerType::Bot);
    TEST("bot difficulty HARD", bot.getDifficulty() == BotDifficulty::Hard);
}

void testGameEngine()
{
    GameConfig cfg;
    cfg.lang = Language::English;

    GameEngine engine(cfg, false);
    engine.init(4);

    engine.addPlayer("Alice", PlayerType::Human, 0);
    engine.addPlayer("Bob", PlayerType::Human, 0);
    engine.addPlayer("Charlie", PlayerType::Human, 0);
    engine.addPlayer("Diana", PlayerType::Human, 0);

    engine.start();

    TEST("game started phase PLAY", engine.getPhase() == GamePhase::Play);
    TEST("4 players in game", engine.getPlayerCount() == 4);
    TEST("no winner yet", engine.getWinner() < 0);
    TEST("not game over", !engine.isGameOver());
    TEST("current turn >= 0", engine.getCurrentTurn() >= 0 && engine.getCurrentTurn() < 4);

    for (int i = 0; i < 4; i++)
    {
        const Player * p = engine.getPlayer(i);
        TEST("player has 7 cards after deal", p->get_size() == 7);
    }

    GameState gs = engine.getState();
    TEST("state direction is 1 or -1", gs.direction == 1 || gs.direction == -1);
    TEST("state player count 4", gs.playerCount == 4);

    int initialTurn = engine.getCurrentTurn();
    engine.nextTurn();
    int newTurn = engine.getCurrentTurn();
    TEST("nextTurn changes player", newTurn != initialTurn || engine.getPhase() == GamePhase::GameOver);

    GameEngine vnEngine(cfg, true);
    vnEngine.init(2);
    vnEngine.addPlayer("X", PlayerType::Human, 0);
    vnEngine.addPlayer("Y", PlayerType::Human, 0);
    vnEngine.start();
    TEST("viet rules engine starts", vnEngine.getPhase() == GamePhase::Play);
}

void testBot()
{
    GameConfig cfg;
    cfg.lang = Language::English;

    GameEngine engine(cfg, false);
    engine.init(2);
    engine.addPlayer("Human", PlayerType::Human, 0);
    engine.addPlayer("Bot", PlayerType::Bot, static_cast<int>(BotDifficulty::Easy));
    engine.start();

    int botIdx = 1;
    const Player * p = engine.getPlayer(botIdx);
    TEST("bot has 7 cards", p->get_size() == 7);
    TEST("bot is bot", p->isBot());

    BotActionResult result = engine.executeBotTurn(botIdx);
    TEST("bot action is valid", result.action == BotAction::PlayCard || result.action == BotAction::Draw);
}

void testRulesIntegration()
{
    {
        Card current(5, CardColor::Red);
        Card play(5, CardColor::Green);
        TEST("matching number", canPlayCard(play, current));
    }

    {
        Card current(5, CardColor::Red);
        Card play(7, CardColor::Red);
        TEST("matching color", canPlayCard(play, current));
    }

    {
        Card current(5, CardColor::Red);
        Card wildCard(13, CardColor::Wild);
        TEST("wild on any", canPlayCard(wildCard, current));
    }

    {
        Card current(13, CardColor::Wild);
        Card play(14, CardColor::Wild);
        TEST("wild on wild", canPlayCard(play, current));
    }

    {
        Card current(13, CardColor::Red);
        Card play(5, CardColor::Red);
        TEST("match declared wild color", canPlayCard(play, current));
    }
}

void testWildDrawFourRule()
{
    // Player has matching color -> cannot play Wild Draw 4
    {
        Player p("Test", PlayerType::Human, BotDifficulty::Easy);
        p.hand_add(Card(5, CardColor::Red));   // matching color
        p.hand_add(Card(14, CardColor::Wild));  // Wild Draw 4

        Card current(3, CardColor::Red);
        Card wd4(14, CardColor::Wild);

        TEST("wd4 blocked when holding matching color",
             !canPlayWildDrawFour(wd4, current, p));
    }

    // Player has NO matching color -> can play Wild Draw 4
    {
        Player p("Test", PlayerType::Human, BotDifficulty::Easy);
        p.hand_add(Card(5, CardColor::Blue));    // non-matching color
        p.hand_add(Card(14, CardColor::Wild));

        Card current(3, CardColor::Red);
        Card wd4(14, CardColor::Wild);

        TEST("wd4 allowed when no matching color",
             canPlayWildDrawFour(wd4, current, p));
    }

    // Player only has other wilds -> can play Wild Draw 4
    {
        Player p("Test", PlayerType::Human, BotDifficulty::Easy);
        p.hand_add(Card(13, CardColor::Wild));    // another wild
        p.hand_add(Card(14, CardColor::Wild));

        Card current(3, CardColor::Red);
        Card wd4(14, CardColor::Wild);

        TEST("wd4 allowed when only wilds",
             canPlayWildDrawFour(wd4, current, p));
    }
}

int main()
{
    std::cout << "=== Running Unit Tests ===" << std::endl << std::endl;

    std::cout << "--- Card & Rules Tests ---" << std::endl;
    testCard();
    testRulesIntegration();
    testWildDrawFourRule();

    std::cout << std::endl << "--- Deck Tests ---" << std::endl;
    testDeck();

    std::cout << std::endl << "--- Player Tests ---" << std::endl;
    testPlayer();

    std::cout << std::endl << "--- Game Engine Tests ---" << std::endl;
    testGameEngine();
    testBot();

    std::cout << std::endl << "=== Results ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
