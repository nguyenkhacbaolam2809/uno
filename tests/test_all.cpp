#include "card.h"
#include "deck.h"
#include "player.h"
#include "rules.h"
#include "game_engine.h"
#include "config.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
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
    card a;
    a.number = 5;
    a.color = red;

    card b;
    b.number = 5;
    b.color = green;

    card c;
    c.number = 7;
    c.color = red;

    card wc;
    wc.number = 13;
    wc.color = ::wild;

    card d4;
    d4.number = 14;
    d4.color = ::wild;

    // Equality by number OR color
    TEST("card same number different color", a == b);
    TEST("card same color different number", a == c);
    TEST("card different number and color", !(b == c));
    TEST("wild match by color wild", wc == d4);

    // canPlayCard tests
    TEST("canPlayCard same number", canPlayCard(b, a));
    TEST("canPlayCard same color", canPlayCard(c, a));
    TEST("canPlayCard wild on anything", canPlayCard(wc, a));
    TEST("canPlayCard wild on wild", canPlayCard(wc, d4));
    TEST("canPlayCard incompatible", !canPlayCard(b, c));

    // isActionCard
    TEST("skip is action", isActionCard(card(11, red)));
    TEST("reverse is action", isActionCard(card(12, green)));
    TEST("draw2 is action", isActionCard(card(10, blue)));
    TEST("wild is action", isActionCard(card(13, ::wild)));
    TEST("wild draw4 is action", isActionCard(card(14, ::wild)));
    TEST("number 5 is not action", !isActionCard(card(5, red)));

    // isStackCard
    TEST("draw2 is stack", isStackCard(card(10, red)));
    TEST("wild draw4 is stack", isStackCard(card(14, ::wild)));
    TEST("skip is not stack", !isStackCard(card(11, red)));
    TEST("number is not stack", !isStackCard(card(5, green)));

    // getStackValue
    TEST("draw2 stack value 2", getStackValue(card(10, blue)) == 2);
    TEST("wild draw4 stack value 4", getStackValue(card(14, ::wild)) == 4);

    // isLegalLastCard (Vietnamese rules)
    TEST("number is legal last", isLegalLastCard(card(3, red)));
    TEST("skip is legal last", isLegalLastCard(card(11, green)));
    TEST("reverse is legal last", isLegalLastCard(card(12, blue)));
    TEST("draw2 not legal last", !isLegalLastCard(card(10, yellow)));
    TEST("wild not legal last", !isLegalLastCard(card(13, ::wild)));
    TEST("wild draw4 not legal last", !isLegalLastCard(card(14, ::wild)));

    // canJumpIn (Vietnamese rules) - requires exact same card (number AND color)
    TEST("jump in exact same card", canJumpIn(card(3, red), card(3, red)));
    TEST("jump in different color", !canJumpIn(card(3, red), card(3, green)));
    TEST("jump in different number", !canJumpIn(card(5, red), card(3, red)));
    TEST("jump in completely different", !canJumpIn(card(5, red), card(3, green)));
}

void testDeck()
{
    deck mainDeck;
    TEST("deck initially empty", mainDeck.get_size() == 0);

    mainDeck.create();
    int initialSize = mainDeck.get_size();

    // Standard UNO deck has 108 cards
    {
        int expected = 108;
        TEST("deck size 108 after create", initialSize == expected);
    }

    // Draw all cards
    for (int i = 0; i < initialSize; i++)
        mainDeck.draw();
    TEST("deck empty after drawing all", mainDeck.get_size() == 0);

    // Draw from empty deck returns invalid card
    card emptyCard = mainDeck.draw();
    TEST("draw from empty returns wild", emptyCard.color == wild);

    // Add card back
    card c(5, red);
    mainDeck.add_card(c);
    TEST("deck has 1 card after add", mainDeck.get_size() == 1);

    card drawn = mainDeck.draw();
    TEST("drawn card matches added", drawn.number == 5 && drawn.color == red);

    // Copy constructor
    mainDeck.create();
    deck copyDeck(mainDeck);
    TEST("copy deck same size", copyDeck.get_size() == mainDeck.get_size());

    // Assignment operator
    deck assignDeck;
    assignDeck = mainDeck;
    TEST("assign deck same size", assignDeck.get_size() == mainDeck.get_size());

    // Shuffle doesn't change size
    mainDeck.quick_shuffle();
    TEST("shuffle preserves size", mainDeck.get_size() == 108);
}

void testPlayer()
{
    player p("TestPlayer", HUMAN, D_EASY);
    TEST("player name", p.getName() == "TestPlayer");
    TEST("player type HUMAN", p.getType() == HUMAN);
    TEST("player difficulty D_EASY", p.getDifficulty() == D_EASY);
    TEST("player isBot false", !p.isBot());
    TEST("player starts empty", p.get_size() == 0);

    card c1(3, red);
    card c2(7, blue);
    card c3(10, green);

    p.hand_add(c1);
    TEST("player has 1 card", p.get_size() == 1);
    TEST("peek matches added", p.peek(0).number == 3 && p.peek(0).color == red);

    p.hand_add(c2);
    p.hand_add(c3);
    TEST("player has 3 cards", p.get_size() == 3);

    // Peek bounds checking
    card invalid = p.peek(-1);
    TEST("peek -1 returns safe card", invalid.color == wild);
    invalid = p.peek(100);
    TEST("peek out of bounds safe", invalid.color == wild);

    // Remove card
    card removed = p.hand_remove(1);
    TEST("removed correct card", removed.number == 7 && removed.color == blue);
    TEST("player has 2 cards after remove", p.get_size() == 2);

    // Remove from invalid position
    removed = p.hand_remove(-1);
    TEST("remove -1 returns safe card", removed.color == wild);
    removed = p.hand_remove(100);
    TEST("remove out of bounds safe", removed.color == wild);

    // Copy constructor
    player p2(p);
    TEST("copy constructor same size", p2.get_size() == p.get_size());

    // Assignment
    player p3;
    p3 = p;
    TEST("assignment same size", p3.get_size() == p.get_size());

    // Bot player
    player bot("Bot1", BOT, D_HARD);
    TEST("bot isBot true", bot.isBot());
    TEST("bot type BOT", bot.getType() == BOT);
    TEST("bot difficulty HARD", bot.getDifficulty() == D_HARD);
}

void testGameEngine()
{
    GameConfig cfg;
    cfg.os = OS_WINDOWS;
    cfg.lang = LANG_ENGLISH;

    GameEngine engine(cfg, false);
    engine.init(4);

    engine.addPlayer("Alice", HUMAN, 0);
    engine.addPlayer("Bob", HUMAN, 0);
    engine.addPlayer("Charlie", HUMAN, 0);
    engine.addPlayer("Diana", HUMAN, 0);

    engine.start();

    TEST("game started phase PLAY", engine.getPhase() == PHASE_PLAY);
    TEST("4 players in game", engine.getPlayerCount() == 4);
    TEST("no winner yet", engine.getWinner() < 0);
    TEST("not game over", !engine.isGameOver());
    TEST("current turn >= 0", engine.getCurrentTurn() >= 0 && engine.getCurrentTurn() < 4);

    // Each player should have 7 cards
    for (int i = 0; i < 4; i++)
    {
        const player * p = engine.getPlayer(i);
        TEST("player has 7 cards after deal", p->get_size() == 7);
    }

    // Test getState
    GameState gs = engine.getState();
    TEST("state direction is 1 or -1", gs.direction == 1 || gs.direction == -1);
    TEST("state player count 4", gs.playerCount == 4);

    // Test turn management
    int initialTurn = engine.getCurrentTurn();
    engine.nextTurn();
    int newTurn = engine.getCurrentTurn();
    TEST("nextTurn changes player", newTurn != initialTurn || engine.getPhase() == PHASE_GAME_OVER);

    // Vietnamese rules
    GameEngine vnEngine(cfg, true);
    vnEngine.init(2);
    vnEngine.addPlayer("X", HUMAN, 0);
    vnEngine.addPlayer("Y", HUMAN, 0);
    vnEngine.start();
    TEST("viet rules engine starts", vnEngine.getPhase() == PHASE_PLAY);
}

void testBot()
{
    GameConfig cfg;
    cfg.os = OS_WINDOWS;
    cfg.lang = LANG_ENGLISH;

    // Test singleplayer mode (1 human + 1 bot)
    GameEngine engine(cfg, false);
    engine.init(2);
    engine.addPlayer("Human", HUMAN, 0);
    engine.addPlayer("Bot", BOT, D_EASY);
    engine.start();

    int botIdx = 1;
    const player * p = engine.getPlayer(botIdx);
    TEST("bot has 7 cards", p->get_size() == 7);
    TEST("bot is bot", p->isBot());

    // Bot can execute a turn
    BotActionResult result = engine.executeBotTurn(botIdx);
    TEST("bot action is valid", result.action == BOT_PLAY_CARD || result.action == BOT_DRAW);
}

void testRulesIntegration()
{
    // Test valid play matching number
    {
        card current(5, red);
        card play(5, green);
        TEST("matching number", canPlayCard(play, current));
    }

    // Test valid play matching color
    {
        card current(5, red);
        card play(7, red);
        TEST("matching color", canPlayCard(play, current));
    }

    // Test wild play on anything
    {
        card current(5, red);
        card wildCard(13, ::wild);
        TEST("wild on any", canPlayCard(wildCard, current));
    }

    // Test wild on wild (matching the wild color)
    {
        card current(13, ::wild);
        card play(14, ::wild);
        TEST("wild on wild", canPlayCard(play, current));
    }

    // Test matching declared color after wild
    {
        card current(13, red);
        card play(5, red);
        TEST("match declared wild color", canPlayCard(play, current));
    }
}

int main()
{
    std::cout << "=== Running Unit Tests ===" << std::endl << std::endl;

    std::cout << "--- Card & Rules Tests ---" << std::endl;
    testCard();
    testRulesIntegration();

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
