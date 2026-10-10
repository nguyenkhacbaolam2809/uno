#include "card.h"
#include "deck.h"
#include "player.h"
#include "rules.h"
#include "game_engine.h"
#include "config.h"
#include "bot_hard.h"
#include "bot_medium.h"
#include "bot_easy.h"
#include "bot_factory.h"
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

void testActionCards()
{
    GameConfig cfg;
    cfg.lang = Language::English;

    // 1. Skip card in 4-player game
    {
        GameEngine engine(cfg, false);
        engine.init(4);
        engine.addPlayer("P0", PlayerType::Human, 0);
        engine.addPlayer("P1", PlayerType::Human, 0);
        engine.addPlayer("P2", PlayerType::Human, 0);
        engine.addPlayer("P3", PlayerType::Human, 0);
        engine.start();

        int t = engine.getCurrentTurn();
        Player * curP = engine.getPlayer(t);
        while (curP->get_size() > 0) curP->hand_remove(0);
        curP->hand_add(Card(CARD_WILD, CardColor::Wild));
        curP->hand_add(Card(9, CardColor::Red));
        engine.playCard(t, 0, CardColor::Red);
        engine.nextTurn();

        int pA = engine.getCurrentTurn();
        Player * pA_ptr = engine.getPlayer(pA);
        while (pA_ptr->get_size() > 0) pA_ptr->hand_remove(0);
        pA_ptr->hand_add(Card(CARD_SKIP, CardColor::Red));
        pA_ptr->hand_add(Card(3, CardColor::Red));

        TEST("can play Skip card", engine.validatePlay(pA, 0));
        engine.playCard(pA, 0, CardColor::Red);
        engine.nextTurn();

        int expectedSkipTurn = (pA + 2) % 4;
        TEST("Skip card skips 1 player", engine.getCurrentTurn() == expectedSkipTurn);
    }

    // 2. Reverse card in 4-player game
    {
        GameEngine engine(cfg, false);
        engine.init(4);
        engine.addPlayer("P0", PlayerType::Human, 0);
        engine.addPlayer("P1", PlayerType::Human, 0);
        engine.addPlayer("P2", PlayerType::Human, 0);
        engine.addPlayer("P3", PlayerType::Human, 0);
        engine.start();

        int t = engine.getCurrentTurn();
        Player * curP = engine.getPlayer(t);
        while (curP->get_size() > 0) curP->hand_remove(0);
        curP->hand_add(Card(CARD_WILD, CardColor::Wild));
        curP->hand_add(Card(9, CardColor::Blue));
        engine.playCard(t, 0, CardColor::Blue);
        engine.nextTurn();

        int pB = engine.getCurrentTurn();
        Player * pB_ptr = engine.getPlayer(pB);
        while (pB_ptr->get_size() > 0) pB_ptr->hand_remove(0);
        pB_ptr->hand_add(Card(CARD_REVERSE, CardColor::Blue));
        pB_ptr->hand_add(Card(4, CardColor::Blue));

        engine.playCard(pB, 0, CardColor::Blue);
        TEST("Reverse flips direction in 4p", engine.getDirection() == -1);
        engine.nextTurn();

        int expectedRevTurn = (pB - 1 + 4) % 4;
        TEST("Turn advances backward after Reverse in 4p", engine.getCurrentTurn() == expectedRevTurn);
    }

    // 3. Reverse card in 2-player game (acts like Skip)
    {
        GameEngine engine2(cfg, false);
        engine2.init(2);
        engine2.addPlayer("P0", PlayerType::Human, 0);
        engine2.addPlayer("P1", PlayerType::Human, 0);
        engine2.start();

        int t = engine2.getCurrentTurn();
        Player * curP = engine2.getPlayer(t);
        while (curP->get_size() > 0) curP->hand_remove(0);
        curP->hand_add(Card(CARD_WILD, CardColor::Wild));
        curP->hand_add(Card(9, CardColor::Yellow));
        engine2.playCard(t, 0, CardColor::Yellow);
        engine2.nextTurn();

        int nextP = engine2.getCurrentTurn();
        Player * pRev = engine2.getPlayer(nextP);
        while (pRev->get_size() > 0) pRev->hand_remove(0);
        pRev->hand_add(Card(CARD_REVERSE, CardColor::Yellow));
        pRev->hand_add(Card(1, CardColor::Yellow));

        engine2.playCard(nextP, 0, CardColor::Yellow);
        engine2.nextTurn();
        TEST("Reverse in 2-player game returns turn to same player", engine2.getCurrentTurn() == nextP);
    }
}

void testDrawStackingAndForceDraw()
{
    GameConfig cfg;
    cfg.lang = Language::English;

    // 1. Draw Two without stacking: target must draw and cannot be bypassed
    {
        GameEngine engine(cfg, false);
        engine.init(4);
        engine.addPlayer("P0", PlayerType::Human, 0);
        engine.addPlayer("P1", PlayerType::Human, 0);
        engine.addPlayer("P2", PlayerType::Human, 0);
        engine.addPlayer("P3", PlayerType::Human, 0);
        engine.start();

        int t0 = engine.getCurrentTurn();
        Player * p0 = engine.getPlayer(t0);
        while (p0->get_size() > 0) p0->hand_remove(0);
        p0->hand_add(Card(CARD_WILD, CardColor::Wild));
        p0->hand_add(Card(9, CardColor::Green));
        engine.playCard(t0, 0, CardColor::Green);
        engine.nextTurn();

        int t1 = engine.getCurrentTurn();
        Player * p1 = engine.getPlayer(t1);
        while (p1->get_size() > 0) p1->hand_remove(0);
        p1->hand_add(Card(CARD_DRAW_TWO, CardColor::Green));
        p1->hand_add(Card(2, CardColor::Green));

        engine.playCard(t1, 0, CardColor::Green);
        TEST("Draw2 adds 2 to drawStack", engine.getDrawStack() == 2);
        TEST("Draw2 sets forceDraw", engine.isForceDraw());

        engine.nextTurn();
        int targetP = (t1 + 1) % 4;
        TEST("Draw2 turn lands on immediate next player", engine.getCurrentTurn() == targetP);

        int preSize = engine.getPlayer(targetP)->get_size();
        engine.drawCard(targetP);
        TEST("Target receives 2 penalty cards", engine.getPlayer(targetP)->get_size() == preSize + 2);
        TEST("forceDraw cleared after draw", !engine.isForceDraw());
        TEST("drawStack cleared after draw", engine.getDrawStack() == 0);

        engine.nextTurn();
        int subsequentP = (targetP + 1) % 4;
        TEST("Turn advances past penalized player", engine.getCurrentTurn() == subsequentP);
        TEST("Subsequent player is not forced to draw", !engine.isForceDraw());
    }

    // 2. Draw Two Stacking: +2 stacked on +2 -> next player faces +4
    {
        GameEngine engine(cfg, false);
        engine.init(3);
        engine.addPlayer("P0", PlayerType::Human, 0);
        engine.addPlayer("P1", PlayerType::Human, 0);
        engine.addPlayer("P2", PlayerType::Human, 0);
        engine.start();

        int t0 = engine.getCurrentTurn();
        Player * p0 = engine.getPlayer(t0);
        while (p0->get_size() > 0) p0->hand_remove(0);
        p0->hand_add(Card(CARD_WILD, CardColor::Wild));
        p0->hand_add(Card(9, CardColor::Red));
        engine.playCard(t0, 0, CardColor::Red);
        engine.nextTurn();

        int t1 = engine.getCurrentTurn();
        Player * p1 = engine.getPlayer(t1);
        while (p1->get_size() > 0) p1->hand_remove(0);
        p1->hand_add(Card(CARD_DRAW_TWO, CardColor::Red));
        p1->hand_add(Card(1, CardColor::Red));

        engine.playCard(t1, 0, CardColor::Red);
        engine.nextTurn();

        int t2 = engine.getCurrentTurn();
        TEST("t2 is immediate next player", t2 == (t1 + 1) % 3);
        TEST("t2 faces forceDraw with stack 2", engine.isForceDraw() && engine.getDrawStack() == 2);

        // t2 stacks another Draw Two (Blue Draw Two matching number 10)
        Player * p2 = engine.getPlayer(t2);
        while (p2->get_size() > 0) p2->hand_remove(0);
        p2->hand_add(Card(CARD_DRAW_TWO, CardColor::Blue));
        p2->hand_add(Card(7, CardColor::Blue));

        TEST("t2 can stack Draw Two", engine.validatePlay(t2, 0));
        engine.playCard(t2, 0, CardColor::Blue);
        TEST("drawStack increased to 4", engine.getDrawStack() == 4);
        engine.nextTurn();

        int t3 = engine.getCurrentTurn();
        TEST("t3 turn faces 4-card stack", t3 == (t2 + 1) % 3);
        TEST("t3 faces forceDraw", engine.isForceDraw());

        int preSize3 = engine.getPlayer(t3)->get_size();
        engine.drawCard(t3);
        TEST("t3 receives 4 stacked cards", engine.getPlayer(t3)->get_size() == preSize3 + 4);
        TEST("forceDraw and stack cleared after 4-card draw", !engine.isForceDraw() && engine.getDrawStack() == 0);
    }
}

void testVietnameseRulesComprehensive()
{
    GameConfig cfg;
    cfg.lang = Language::English;

    // 1. Illegal last card restrictions
    {
        GameEngine vnEngine(cfg, true);
        vnEngine.init(2);
        vnEngine.addPlayer("P0", PlayerType::Human, 0);
        vnEngine.addPlayer("P1", PlayerType::Human, 0);
        vnEngine.start();

        int cur = vnEngine.getCurrentTurn();
        Player * p0 = vnEngine.getPlayer(cur);
        while (p0->get_size() > 0) p0->hand_remove(0);
        p0->hand_add(Card(CARD_WILD, CardColor::Wild));
        p0->hand_add(Card(9, CardColor::Red));
        vnEngine.playCard(cur, 0, CardColor::Red);
        vnEngine.nextTurn();

        int nextT = vnEngine.getCurrentTurn();
        Player * p1 = vnEngine.getPlayer(nextT);
        while (p1->get_size() > 0) p1->hand_remove(0);
        p1->hand_add(Card(5, CardColor::Red));
        p1->hand_add(Card(9, CardColor::Red));
        vnEngine.playCard(nextT, 0, CardColor::Red);
        vnEngine.nextTurn();

        int activeP = vnEngine.getCurrentTurn();
        Player * pAct = vnEngine.getPlayer(activeP);
        while (pAct->get_size() > 0) pAct->hand_remove(0);

        // Draw Two as last card: rejected
        pAct->hand_add(Card(CARD_DRAW_TWO, CardColor::Red));
        TEST("VN rules reject Draw2 as last card in validatePlay", !vnEngine.validatePlay(activeP, 0));
        TEST("VN rules reject Draw2 as last card in playCard", !vnEngine.playCard(activeP, 0, CardColor::Red));
        TEST("Player not won on illegal last card", !vnEngine.isGameOver());

        // Wild as last card: rejected
        pAct->hand_remove(0);
        pAct->hand_add(Card(CARD_WILD, CardColor::Wild));
        TEST("VN rules reject Wild as last card in validatePlay", !vnEngine.validatePlay(activeP, 0));
        TEST("VN rules reject Wild as last card in playCard", !vnEngine.playCard(activeP, 0, CardColor::Yellow));

        // Wild Draw Four as last card: rejected
        pAct->hand_remove(0);
        pAct->hand_add(Card(CARD_WILD_DRAW_FOUR, CardColor::Wild));
        TEST("VN rules reject WD4 as last card in validatePlay", !vnEngine.validatePlay(activeP, 0));
        TEST("VN rules reject WD4 as last card in playCard", !vnEngine.playCard(activeP, 0, CardColor::Yellow));

        // Number card as last card: accepted and wins
        pAct->hand_remove(0);
        pAct->hand_add(Card(8, CardColor::Red));
        TEST("VN rules accept Number card as last card", vnEngine.validatePlay(activeP, 0));
        TEST("playCard succeeds with legal last card", vnEngine.playCard(activeP, 0, CardColor::Red));
        TEST("Player wins game with legal last card", vnEngine.isGameOver() && vnEngine.getWinner() == activeP);
    }

    // 2. Jump-In mechanics
    {
        GameEngine jEngine(cfg, true);
        jEngine.init(3);
        jEngine.addPlayer("A", PlayerType::Human, 0);
        jEngine.addPlayer("B", PlayerType::Human, 0);
        jEngine.addPlayer("C", PlayerType::Human, 0);
        jEngine.start();

        int jCur = jEngine.getCurrentTurn();
        Player * pJ0 = jEngine.getPlayer(jCur);
        while (pJ0->get_size() > 0) pJ0->hand_remove(0);
        pJ0->hand_add(Card(CARD_WILD, CardColor::Wild));
        pJ0->hand_add(Card(1, CardColor::Blue));
        jEngine.playCard(jCur, 0, CardColor::Green);
        jEngine.nextTurn();

        int jNext = jEngine.getCurrentTurn();
        Player * pJ1 = jEngine.getPlayer(jNext);
        while (pJ1->get_size() > 0) pJ1->hand_remove(0);
        pJ1->hand_add(Card(6, CardColor::Green));
        pJ1->hand_add(Card(2, CardColor::Green));
        jEngine.playCard(jNext, 0, CardColor::Green);

        int otherP = (jNext + 2) % 3;
        Player * pOther = jEngine.getPlayer(otherP);
        while (pOther->get_size() > 0) pOther->hand_remove(0);
        pOther->hand_add(Card(6, CardColor::Green));
        pOther->hand_add(Card(9, CardColor::Green));

        TEST("jumpIn succeeds with exact matching card", jEngine.jumpIn(otherP, 0));
        TEST("jumpState is Taken", jEngine.getState().jumpState == JumpInState::Taken);

        jEngine.nextTurn();
        int expectedNext = (otherP + 1) % 3;
        TEST("Turn resumes after jumper", jEngine.getCurrentTurn() == expectedNext);
    }

    // 3. UNO call and catch
    {
        GameEngine uEngine(cfg, true);
        uEngine.init(2);
        uEngine.addPlayer("U0", PlayerType::Human, 0);
        uEngine.addPlayer("U1", PlayerType::Human, 0);
        uEngine.start();

        int uCur = uEngine.getCurrentTurn();
        Player * uP0 = uEngine.getPlayer(uCur);
        while (uP0->get_size() > 0) uP0->hand_remove(0);
        uP0->hand_add(Card(CARD_WILD, CardColor::Wild));
        uP0->hand_add(Card(9, CardColor::Red));
        uEngine.playCard(uCur, 0, CardColor::Red);
        uEngine.nextTurn();

        int uNext = uEngine.getCurrentTurn();
        Player * uP1 = uEngine.getPlayer(uNext);
        while (uP1->get_size() > 0) uP1->hand_remove(0);
        uP1->hand_add(Card(1, CardColor::Red));
        uP1->hand_add(Card(9, CardColor::Red));
        uEngine.playCard(uNext, 0, CardColor::Red);
        uEngine.nextTurn();

        // Case A: U0 plays down to 1 card and calls UNO
        int pTurn = uEngine.getCurrentTurn();
        int pOther = (pTurn + 1) % 2;
        Player * pMain = uEngine.getPlayer(pTurn);
        while (pMain->get_size() > 0) pMain->hand_remove(0);
        pMain->hand_add(Card(1, CardColor::Blue));
        pMain->hand_add(Card(2, CardColor::Blue));

        uEngine.callUno(pTurn);
        uEngine.playCard(pTurn, 0, CardColor::Blue);
        uEngine.nextTurn();
        TEST("pMain has 1 card left", pMain->get_size() == 1);

        int otherSizeBefore = uEngine.getPlayer(pOther)->get_size();
        // pOther attempts false catch:
        uEngine.catchUno(pOther, pTurn);
        TEST("Protected target retains 1 card on false catch", pMain->get_size() == 1);
        TEST("False accuser penalized with 2 cards", uEngine.getPlayer(pOther)->get_size() == otherSizeBefore + 2);

        // Case B: Player plays down to 1 card without calling UNO:
        Player * pVictim = uEngine.getPlayer(pOther);
        while (pVictim->get_size() > 0) pVictim->hand_remove(0);
        pVictim->hand_add(Card(1, CardColor::Green));
        pVictim->hand_add(Card(3, CardColor::Green));
        // Does not call UNO:
        uEngine.playCard(pOther, 0, CardColor::Green);
        uEngine.nextTurn();

        // Accuser catches victim:
        uEngine.catchUno(pTurn, pOther);
        TEST("Victim penalized 2 cards for forgetting UNO", pVictim->get_size() == 3);

        // Bounds safety
        uEngine.catchUno(-1, pOther);
        uEngine.catchUno(pTurn, -1);
        uEngine.catchUno(100, 200);
        TEST("catchUno handles out-of-bounds indices safely", true);
    }
}

void testDeckReshuffleLogic()
{
    GameConfig cfg;
    cfg.lang = Language::English;

    GameEngine engine(cfg, false);
    engine.init(2);
    engine.addPlayer("P0", PlayerType::Human, 0);
    engine.addPlayer("P1", PlayerType::Human, 0);
    engine.start();

    // Draw cards until initial deck is exhausted and discard reshuffles
    int drawCount = 0;
    while (drawCount < 110)
    {
        engine.drawCard(0);
        drawCount++;
    }

    TEST("P0 successfully drew cards through deck exhaustion", engine.getPlayer(0)->get_size() > 10);
    TEST("currentCard remains valid after reshuffle", engine.getCurrentCard().number >= 0);
}

void testBotAIStrategiesComprehensive()
{
    GameConfig cfg;
    cfg.lang = Language::English;

    // 1. HardBot stack defense
    {
        HardBotStrategy bot;
        Player self("Bot", PlayerType::Bot, BotDifficulty::Hard);
        self.hand_add(Card(5, CardColor::Red));
        self.hand_add(Card(CARD_DRAW_TWO, CardColor::Red));

        Card current(CARD_DRAW_TWO, CardColor::Red);
        int oppSizes[5] = {0};
        int defIdx = bot.pickCard(&self, current, 1, 0, oppSizes, 2, 2);
        TEST("HardBot detects stack defense card", defIdx == 1);
    }

    // 2. HardBot threat blocking
    {
        HardBotStrategy bot;
        Player self("Bot", PlayerType::Bot, BotDifficulty::Hard);
        self.hand_add(Card(3, CardColor::Red));
        self.hand_add(Card(CARD_SKIP, CardColor::Red));

        Card current(5, CardColor::Red);
        int oppSizes[5] = {5, 1};
        int blockIdx = bot.pickCard(&self, current, 1, 0, oppSizes, 2, 0);
        TEST("HardBot blocks threatening opponent holding 1 card", blockIdx == 1);
    }

    // 3. Bot under Vietnamese rules doesn't play illegal last card
    {
        GameEngine bEngine(cfg, true);
        bEngine.init(2);
        bEngine.addPlayer("Human", PlayerType::Human, 0);
        bEngine.addPlayer("Bot", PlayerType::Bot, static_cast<int>(BotDifficulty::Hard));
        bEngine.start();

        Player * bPtr = bEngine.getPlayer(1);
        while (bPtr->get_size() > 0) bPtr->hand_remove(0);
        bPtr->hand_add(Card(CARD_WILD, CardColor::Wild)); // 1 card left, illegal last card

        BotActionResult act = bEngine.executeBotTurn(1);
        TEST("Bot falls back to draw when only holding illegal last card", act.action == BotAction::Draw);
    }

    // 4. Bot Factory creates correct difficulties
    {
        auto easy = createBotStrategy(BotDifficulty::Easy);
        auto normal = createBotStrategy(BotDifficulty::Normal);
        auto hard = createBotStrategy(BotDifficulty::Hard);
        TEST("Bot factory creates easy bot", easy != nullptr);
        TEST("Bot factory creates normal bot", normal != nullptr);
        TEST("Bot factory creates hard bot", hard != nullptr);
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
    testDeckReshuffleLogic();

    std::cout << std::endl << "--- Player Tests ---" << std::endl;
    testPlayer();

    std::cout << std::endl << "--- Game Engine Tests ---" << std::endl;
    testGameEngine();
    testActionCards();
    testDrawStackingAndForceDraw();
    testVietnameseRulesComprehensive();

    std::cout << std::endl << "--- Bot AI Tests ---" << std::endl;
    testBot();
    testBotAIStrategiesComprehensive();

    std::cout << std::endl << "=== Results ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
