#include "console_ui.h"
#include "rules.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <limits>
#include <string>

ConsoleUI::ConsoleUI(const GameConfig & cfg)
    : config(cfg), vietRules(false)
{
}

ConsoleUI::~ConsoleUI()
{
}

void ConsoleUI::waitForEnter()
{
    std::cout << msg(config, 43);
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

void ConsoleUI::showIntro()
{
    std::string filename = (config.lang == LANG_VIETNAMESE) ? "assets/intro_vi.txt" : "assets/intro.txt";
    std::string line;
    std::fstream myfile;
    myfile.open(filename.c_str());
    if (myfile.is_open())
    {
        while (getline(myfile, line))
            std::cout << line << std::endl;
        myfile.close();
    }
}

bool ConsoleUI::askVietnameseRules()
{
    std::string ans;
    std::cout << msg(config, 51);
    std::cin >> ans;
    if (ans == "y" || ans == "c")
    {
        std::cout << msg(config, 52) << std::endl;
        return true;
    }
    return false;
}

void ConsoleUI::showMainMenu()
{
    clearScreen(config);
    std::cout << msg(config, 24) << std::endl;
    std::cout << msg(config, 25) << std::endl;
    std::cout << msg(config, 26) << std::endl;
    std::cout << msg(config, 27) << std::endl;
    std::cout << msg(config, 28) << std::endl;
    std::cout << msg(config, 29) << std::endl;
    std::cout << msg(config, 30);
}

void ConsoleUI::showTurnHeader(player & p, const GameEngine & engine)
{
    std::cout << "=== " << msg(config, 5) << " " << p.getName() << " ===" << std::endl;
}

void ConsoleUI::showGameStatus(const GameEngine & engine)
{
    std::cout << msg(config, 12) << std::endl;
    for (int i = 0; i < engine.getPlayerCount(); i++)
    {
        const player * p = engine.getPlayer(i);
        std::cout << p->getName() << ": " << p->get_size() << "   ";
    }
    std::cout << std::endl;
    std::cout << msg(config, 13) << engine.getCurrentCard() << std::endl;
}

int ConsoleUI::pickCardFromHand(player & p, const card & current)
{
    while (true)
    {
        std::cout << msg(config, 14) << std::endl;
        std::cout << msg(config, 15) << std::endl;

        p.print();
        std::cout << msg(config, 16);

        int idx;
        std::cin >> idx;
        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << msg(config, 22) << std::endl;
            continue;
        }

        if (idx == -1)
            return -1;
        if (idx < 0 || idx >= p.get_size())
        {
            std::cout << msg(config, 22) << std::endl;
            continue;
        }

        card chosen = p.peek(idx);
        if (canPlayCard(chosen, current))
            return idx;

        std::cout << msg(config, 21) << std::endl;
    }
}

std::string ConsoleUI::pickColor()
{
    std::string str;
    while (true)
    {
        std::cout << msg(config, 19);
        std::cin >> str;
        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (str == "do" || str == "red") return str;
        if (str == "xanh la" || str == "green") return str;
        if (str == "xanh duong" || str == "blue") return str;
        if (str == "vang" || str == "yellow") return str;
        std::cout << msg(config, 20) << std::endl;
    }
}

void ConsoleUI::handleHumanTurn(GameEngine & engine, int playerIdx)
{
    player * p = engine.getPlayer(playerIdx);
    card currentCard = engine.getCurrentCard();
    bool hasForceDraw = engine.isForceDraw();

    if (hasForceDraw)
    {
        std::cout << msg(config, 10) << " x" << engine.getDrawStack() << std::endl;
        engine.drawCard(playerIdx);

        bool canStack = false;
        for (int i = 0; i < p->get_size(); i++)
        {
            card c = p->peek(i);
            if (isStackCard(c) && canPlayCard(c, currentCard))
            {
                canStack = true;
                break;
            }
        }

        if (canStack)
        {
            int idx = pickCardFromHand(*p, currentCard);
            if (idx == -1) return;
            card chosen = p->peek(idx);
            if (isStackCard(chosen) && canPlayCard(chosen, currentCard))
            {
                std::string col;
                if (chosen.color == wild)
                    col = pickColor();
                engine.playCard(playerIdx, idx, col);
                return;
            }
        }
        return;
    }

    int idx = pickCardFromHand(*p, currentCard);
    if (idx == -1)
    {
        std::cout << msg(config, 42) << std::endl;
        engine.drawCard(playerIdx);

        int lastIdx = p->get_size() - 1;
        card drawn = p->peek(lastIdx);
        if (canPlayCard(drawn, currentCard))
        {
            std::cout << msg(config, 17) << drawn << std::endl;
            std::string ans;
            std::cout << msg(config, 18);
            std::cin >> ans;
            if (ans == "y" || ans == "c")
            {
                std::string col;
                if (drawn.color == wild)
                    col = pickColor();
                engine.playCard(playerIdx, lastIdx, col);
            }
        }
        return;
    }

    card chosen = p->peek(idx);
    std::string colorName;
    if (chosen.color == wild)
        colorName = pickColor();

    if (vietRules && p->get_size() == 2)
    {
        std::cout << p->getName() << ": " << msg(config, 44) << "? ";
        std::string unoAns;
        std::cin >> unoAns;
        if (unoAns != "y" && unoAns != "c")
        {
            std::cout << msg(config, 45) << std::endl;
            engine.drawCard(playerIdx);
            engine.drawCard(playerIdx);
        }
    }

    engine.playCard(playerIdx, idx, colorName);
}

void ConsoleUI::handleBotTurn(GameEngine & engine, int playerIdx)
{
    player * p = engine.getPlayer(playerIdx);
    BotActionResult act = engine.executeBotTurn(playerIdx);

    switch (act.action)
    {
        case BOT_PLAY_CARD:
        case BOT_STACK_CARD:
        {
            card chosen = p->peek(act.cardIdx);
            std::string col;
            if (chosen.color == wild)
            {
                if (act.chosenColor == red) col = "do";
                else if (act.chosenColor == green) col = "xanh la";
                else if (act.chosenColor == blue) col = "xanh duong";
                else if (act.chosenColor == yellow) col = "vang";
                else col = "do";
            }
            engine.playCard(playerIdx, act.cardIdx, col);
            break;
        }
        case BOT_DRAW:
        {
            engine.drawCard(playerIdx);
            int lastIdx = p->get_size() - 1;
            card drawn = p->peek(lastIdx);
            if (canPlayCard(drawn, engine.getCurrentCard()))
            {
                std::string col;
                if (drawn.color == wild)
                {
                    if (act.chosenColor == red) col = "do";
                    else if (act.chosenColor == green) col = "xanh la";
                    else if (act.chosenColor == blue) col = "xanh duong";
                    else if (act.chosenColor == yellow) col = "vang";
                    else col = "do";
                }
                engine.playCard(playerIdx, lastIdx, col);
            }
            break;
        }
    }
}

void ConsoleUI::processGameLoop(GameEngine & engine)
{
    engine.start();

    while (!engine.isGameOver())
    {
        clearScreen(config);
        int playerIdx = engine.getCurrentTurn() % engine.getPlayerCount();
        player * p = engine.getPlayer(playerIdx);

        showTurnHeader(*p, engine);
        showGameStatus(engine);

        if (p->isBot())
        {
            std::cout << msg(config, 42) << std::endl;
            handleBotTurn(engine, playerIdx);
        }
        else
        {
            handleHumanTurn(engine, playerIdx);
        }

        if (engine.isGameOver())
            break;

        engine.nextTurn();
        waitForEnter();
    }

    int winner = engine.getWinner();
    if (winner >= 0)
    {
        clearScreen(config);
        std::cout << engine.getPlayer(winner)->getName() << msg(config, 23) << std::endl;
    }
}

void ConsoleUI::modeSinglePlayer()
{
    clearScreen(config);
    std::cout << "=== " << msg(config, 25) << " ===" << std::endl;

    int diff;
    std::cout << msg(config, 32) << std::endl;
    std::cout << msg(config, 33) << std::endl;
    std::cout << msg(config, 34) << std::endl;
    std::cout << msg(config, 35) << std::endl;
    std::cout << msg(config, 36);
    std::cin >> diff;
    if (diff < 1 || diff > 3) diff = 1;

    int numBots;
    std::cout << msg(config, 39);
    std::cin >> numBots;
    if (numBots < 1) numBots = 1;
    if (numBots > 4) numBots = 4;

    int total = numBots + 1;
    GameEngine engine(config, vietRules);
    engine.init(total);

    std::string name;
    std::cout << msg(config, 37);
    std::cin >> name;
    engine.addPlayer(name, HUMAN, 0);

    for (int i = 1; i < total; i++)
        engine.addPlayer(msg(config, 40) + " " + std::to_string(i), BOT, diff - 1);

    processGameLoop(engine);
}

void ConsoleUI::modeLocalMultiplayer()
{
    clearScreen(config);
    std::cout << "=== " << msg(config, 26) << " ===" << std::endl;

    int amount;
    std::cout << msg(config, 1);
    std::cin >> amount;
    if (amount < 2) amount = 2;
    if (amount > 5) amount = 5;

    GameEngine engine(config, vietRules);
    engine.init(amount);

    for (int i = 0; i < amount; i++)
    {
        std::string name;
        std::cout << msg(config, 37);
        std::cin >> name;
        engine.addPlayer(name, HUMAN, 0);
    }

    processGameLoop(engine);
}

void ConsoleUI::modeMixed()
{
    clearScreen(config);
    std::cout << "=== " << msg(config, 27) << " ===" << std::endl;

    int diff;
    std::cout << msg(config, 32) << std::endl;
    std::cout << msg(config, 33) << std::endl;
    std::cout << msg(config, 34) << std::endl;
    std::cout << msg(config, 35) << std::endl;
    std::cout << msg(config, 36);
    std::cin >> diff;
    if (diff < 1 || diff > 3) diff = 1;

    int numHumans, numBots;
    std::cout << msg(config, 38);
    std::cin >> numHumans;
    if (numHumans < 1) numHumans = 1;
    if (numHumans > 4) numHumans = 4;

    int maxBots = 5 - numHumans;
    std::cout << msg(config, 39);
    std::cin >> numBots;
    if (numBots < 1) numBots = 1;
    if (numBots > maxBots) numBots = maxBots;

    int total = numHumans + numBots;
    GameEngine engine(config, vietRules);
    engine.init(total);

    for (int i = 0; i < numHumans; i++)
    {
        std::string name;
        std::cout << msg(config, 37);
        std::cin >> name;
        engine.addPlayer(name, HUMAN, 0);
    }
    for (int i = 0; i < numBots; i++)
        engine.addPlayer(msg(config, 40) + " " + std::to_string(i + 1), BOT, diff - 1);

    processGameLoop(engine);
}

void ConsoleUI::modeLanServer()
{
    clearScreen(config);
    std::cout << "=== " << msg(config, 28) << " ===" << std::endl;

    int port = DEFAULT_PORT;
    std::cout << msg(config, 58);
    std::string portStr;
    std::cin >> portStr;
    if (!portStr.empty())
        port = std::atoi(portStr.c_str());
    if (port <= 0) port = DEFAULT_PORT;

    NetworkServer server(config, vietRules);
    if (!server.start(port))
    {
        std::cout << "Failed to start server!" << std::endl;
        waitForEnter();
        return;
    }

    int numPlayers;
    std::cout << msg(config, 1);
    std::cin >> numPlayers;
    if (numPlayers < 2) numPlayers = 2;
    if (numPlayers > 5) numPlayers = 5;

    std::cout << msg(config, 55) << std::endl;

    int remotePlayers = numPlayers - 1;
    if (!server.waitForPlayers(remotePlayers))
        return;

    server.runGameLoop();
    waitForEnter();
}

void ConsoleUI::modeLanClient()
{
    clearScreen(config);
    std::cout << "=== " << msg(config, 28) << " ===" << std::endl;

    std::string ip;
    std::cout << msg(config, 57);
    std::cin >> ip;

    int port = DEFAULT_PORT;
    std::cout << msg(config, 58);
    std::string portStr;
    std::cin >> portStr;
    if (!portStr.empty())
        port = std::atoi(portStr.c_str());
    if (port <= 0) port = DEFAULT_PORT;

    NetworkClient client;
    if (!client.connect(ip, port))
    {
        std::cout << "Connection failed!" << std::endl;
        waitForEnter();
        return;
    }
    std::cout << msg(config, 54) << std::endl;

    int myPlayerId = 0;
    SyncState state;

    while (client.isConnected())
    {
        if (!client.receiveSyncState(state, 5000))
            continue;

        myPlayerId = state.myPlayerId;
        clearScreen(config);

        if (state.gs.phase == PHASE_GAME_OVER)
        {
            std::cout << state.players[state.gs.winner].name
                 << msg(config, 23) << std::endl;
            waitForEnter();
            break;
        }

        std::cout << msg(config, 13) << state.gs.currentCard << std::endl;
        std::cout << std::endl;

        for (std::size_t i = 0; i < state.players.size(); i++)
        {
            if ((int)i == myPlayerId)
            {
                std::cout << state.players[i].name << " (YOU)"
                     << ": " << state.players[i].hand.size() << " cards"
                     << std::endl;
                std::cout << msg(config, 5) << i + 1 << " ";
                for (std::size_t j = 0; j < state.players[i].hand.size(); j++)
                    std::cout << j << ": " << state.players[i].hand[j] << "  ";
                std::cout << std::endl;
            }
            else
            {
                std::cout << state.players[i].name
                     << ": " << state.players[i].hand.size() << " cards"
                     << std::endl;
            }
        }
        std::cout << std::endl;

        if (state.gs.turn % (int)state.players.size() != myPlayerId)
        {
            std::cout << msg(config, 42) << std::endl;
            waitForEnter();
            continue;
        }

        if (state.gs.forceDraw)
        {
            std::cout << msg(config, 10) << " x" << state.gs.drawStack << std::endl;
            client.sendDraw(myPlayerId);
            waitForEnter();
            continue;
        }

        const SyncPlayer & me = state.players[myPlayerId];
        card currentCard = state.gs.currentCard;
        bool played = false;

        for (std::size_t j = 0; j < me.hand.size(); j++)
        {
            if (canPlayCard(me.hand[j], currentCard))
            {
                played = true;
                break;
            }
        }

        if (!played)
        {
            std::cout << msg(config, 42) << std::endl;
            client.sendDraw(myPlayerId);
            waitForEnter();
            continue;
        }

        std::cout << msg(config, 14) << std::endl;
        std::cout << msg(config, 16);

        int idx;
        std::cin >> idx;

        if (idx >= 0 && idx < (int)me.hand.size())
        {
            card chosen = me.hand[idx];
            if (canPlayCard(chosen, currentCard))
            {
                std::string col;
                if (chosen.color == wild)
                    col = pickColor();
                client.sendPlayCard(idx, col, myPlayerId);
            }
        }

        waitForEnter();
    }

    client.disconnect();
}

void ConsoleUI::modeLan()
{
    clearScreen(config);
    std::cout << "=== " << msg(config, 28) << " ===" << std::endl;
    std::cout << msg(config, 56);
    int choice;
    std::cin >> choice;

    if (choice == 1)
        modeLanServer();
    else
        modeLanClient();
}

void ConsoleUI::run()
{
    showIntro();
    vietRules = askVietnameseRules();
    waitForEnter();

    while (true)
    {
        showMainMenu();
        int choice;
        std::cin >> choice;
        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = -1;
        }

        switch (choice)
        {
            case 1: modeSinglePlayer(); break;
            case 2: modeLocalMultiplayer(); break;
            case 3: modeMixed(); break;
            case 4: modeLan(); break;
            case 5: return;
            default:
                std::cout << msg(config, 31) << std::endl;
                waitForEnter();
                continue;
        }

        if (choice >= 1 && choice <= 4)
            waitForEnter();
    }
}
