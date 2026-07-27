#include "console_ui.h"
#include "rules.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <limits>
using namespace std;

ConsoleUI::ConsoleUI(const GameConfig & cfg)
    : config(cfg), vietRules(false)
{
}

ConsoleUI::~ConsoleUI()
{
}

void ConsoleUI::waitForEnter()
{
    cout << msg(config, 43);
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

void ConsoleUI::showIntro()
{
    string filename = (config.lang == LANG_VIETNAMESE) ? "assets/intro_vi.txt" : "assets/intro.txt";
    string line;
    fstream myfile;
    myfile.open(filename.c_str());
    if (myfile.is_open())
    {
        while (getline(myfile, line))
            cout << line << endl;
        myfile.close();
    }
}

bool ConsoleUI::askVietnameseRules()
{
    string ans;
    cout << msg(config, 51);
    cin >> ans;
    if (ans == "y" || ans == "c")
    {
        cout << msg(config, 52) << endl;
        return true;
    }
    return false;
}

void ConsoleUI::showMainMenu()
{
    clearScreen(config);
    cout << msg(config, 24) << endl;
    cout << msg(config, 25) << endl;
    cout << msg(config, 26) << endl;
    cout << msg(config, 27) << endl;
    cout << msg(config, 28) << endl;
    cout << msg(config, 29) << endl;
    cout << msg(config, 30);
}

void ConsoleUI::showTurnHeader(player & p, const GameEngine & engine)
{
    cout << "=== " << msg(config, 5) << " " << p.getName() << " ===" << endl;
}

void ConsoleUI::showGameStatus(const GameEngine & engine)
{
    cout << msg(config, 12) << endl;
    for (int i = 0; i < engine.getPlayerCount(); i++)
    {
        const player * p = engine.getPlayer(i);
        cout << p->getName() << ": " << p->get_size() << "   ";
    }
    cout << endl;
    cout << msg(config, 13) << engine.getCurrentCard() << endl;
}

int ConsoleUI::pickCardFromHand(player & p, const card & current, bool forceDraw)
{
    while (true)
    {
        cout << msg(config, 14) << endl;
        cout << msg(config, 15) << endl;

        p.print();
        cout << msg(config, 16);

        int idx;
        cin >> idx;
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << msg(config, 22) << endl;
            continue;
        }

        if (idx == -1)
            return -1;
        if (idx < 0 || idx >= p.get_size())
        {
            cout << msg(config, 22) << endl;
            continue;
        }

        card chosen = p.peek(idx);
        if (canPlayCard(chosen, current))
            return idx;

        cout << msg(config, 21) << endl;
    }
}

string ConsoleUI::pickColor()
{
    string str;
    while (true)
    {
        cout << msg(config, 19);
        cin >> str;
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        if (str == "do" || str == "red") return str;
        if (str == "xanh la" || str == "green") return str;
        if (str == "xanh duong" || str == "blue") return str;
        if (str == "vang" || str == "yellow") return str;
        cout << msg(config, 20) << endl;
    }
}

void ConsoleUI::handleHumanTurn(GameEngine & engine, int playerIdx)
{
    player * p = engine.getPlayer(playerIdx);
    card currentCard = engine.getCurrentCard();
    bool hasForceDraw = engine.isForceDraw();

    if (hasForceDraw)
    {
        cout << msg(config, 10) << " x" << engine.getDrawStack() << endl;
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
            int idx = pickCardFromHand(*p, currentCard, true);
            if (idx == -1) return;
            card chosen = p->peek(idx);
            if (isStackCard(chosen) && canPlayCard(chosen, currentCard))
            {
                string col;
                if (chosen.color == wild)
                    col = pickColor();
                engine.playCard(playerIdx, idx, col);
                return;
            }
        }
        return;
    }

    int idx = pickCardFromHand(*p, currentCard, false);
    if (idx == -1)
    {
        cout << msg(config, 42) << endl;
        engine.drawCard(playerIdx);

        int lastIdx = p->get_size() - 1;
        card drawn = p->peek(lastIdx);
        if (canPlayCard(drawn, currentCard))
        {
            cout << msg(config, 17) << drawn << endl;
            string ans;
            cout << msg(config, 18);
            cin >> ans;
            if (ans == "y" || ans == "c")
            {
                string col;
                if (drawn.color == wild)
                    col = pickColor();
                engine.playCard(playerIdx, lastIdx, col);
            }
        }
        return;
    }

    card chosen = p->peek(idx);
    string colorName;
    if (chosen.color == wild)
        colorName = pickColor();

    if (vietRules && p->get_size() == 1)
    {
        cout << p->getName() << ": " << msg(config, 44) << endl;
        string unoAns;
        cin >> unoAns;
        if (unoAns != "y" && unoAns != "c")
        {
            cout << msg(config, 45) << endl;
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
        {
            card chosen = p->peek(act.cardIdx);
            string col;
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
        case BOT_STACK_CARD:
        {
            card chosen = p->peek(act.cardIdx);
            string col;
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
                string col;
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
            cout << msg(config, 42) << endl;
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
        cout << engine.getPlayer(winner)->getName() << msg(config, 23) << endl;
    }
}

void ConsoleUI::modeSinglePlayer()
{
    clearScreen(config);
    cout << "=== " << msg(config, 25) << " ===" << endl;

    int diff;
    cout << msg(config, 32) << endl;
    cout << msg(config, 33) << endl;
    cout << msg(config, 34) << endl;
    cout << msg(config, 35) << endl;
    cout << msg(config, 36);
    cin >> diff;
    if (diff < 1 || diff > 3) diff = 1;

    int numBots;
    cout << msg(config, 39);
    cin >> numBots;
    if (numBots < 1) numBots = 1;
    if (numBots > 4) numBots = 4;

    int total = numBots + 1;
    GameEngine engine(config, vietRules);
    engine.init(total);

    string name;
    cout << msg(config, 37);
    cin >> name;
    engine.addPlayer(name, HUMAN, 0);

    for (int i = 1; i < total; i++)
        engine.addPlayer(msg(config, 40) + " " + to_string(i), BOT, diff - 1);

    processGameLoop(engine);
}

void ConsoleUI::modeLocalMultiplayer()
{
    clearScreen(config);
    cout << "=== " << msg(config, 26) << " ===" << endl;

    int amount;
    cout << msg(config, 1);
    cin >> amount;
    if (amount < 2) amount = 2;
    if (amount > 5) amount = 5;

    GameEngine engine(config, vietRules);
    engine.init(amount);

    for (int i = 0; i < amount; i++)
    {
        string name;
        cout << msg(config, 37);
        cin >> name;
        engine.addPlayer(name, HUMAN, 0);
    }

    processGameLoop(engine);
}

void ConsoleUI::modeMixed()
{
    clearScreen(config);
    cout << "=== " << msg(config, 27) << " ===" << endl;

    int diff;
    cout << msg(config, 32) << endl;
    cout << msg(config, 33) << endl;
    cout << msg(config, 34) << endl;
    cout << msg(config, 35) << endl;
    cout << msg(config, 36);
    cin >> diff;
    if (diff < 1 || diff > 3) diff = 1;

    int numHumans, numBots;
    cout << msg(config, 38);
    cin >> numHumans;
    if (numHumans < 1) numHumans = 1;
    if (numHumans > 4) numHumans = 4;

    int maxBots = 5 - numHumans;
    cout << msg(config, 39);
    cin >> numBots;
    if (numBots < 1) numBots = 1;
    if (numBots > maxBots) numBots = maxBots;

    int total = numHumans + numBots;
    GameEngine engine(config, vietRules);
    engine.init(total);

    for (int i = 0; i < numHumans; i++)
    {
        string name;
        cout << msg(config, 37);
        cin >> name;
        engine.addPlayer(name, HUMAN, 0);
    }
    for (int i = 0; i < numBots; i++)
        engine.addPlayer(msg(config, 40) + " " + to_string(i + 1), BOT, diff - 1);

    processGameLoop(engine);
}

void ConsoleUI::modeLanServer()
{
    clearScreen(config);
    cout << "=== " << msg(config, 28) << " ===" << endl;

    int port = DEFAULT_PORT;
    cout << msg(config, 58);
    string portStr;
    cin >> portStr;
    if (!portStr.empty())
        port = atoi(portStr.c_str());
    if (port <= 0) port = DEFAULT_PORT;

    NetworkServer server(config, vietRules);
    if (!server.start(port))
    {
        cout << "Failed to start server!" << endl;
        waitForEnter();
        return;
    }

    int numPlayers;
    cout << msg(config, 1);
    cin >> numPlayers;
    if (numPlayers < 2) numPlayers = 2;
    if (numPlayers > 5) numPlayers = 5;

    cout << msg(config, 55) << endl;

    int remotePlayers = numPlayers - 1;
    if (!server.waitForPlayers(remotePlayers))
        return;

    server.runGameLoop();
    waitForEnter();
}

void ConsoleUI::modeLanClient()
{
    clearScreen(config);
    cout << "=== " << msg(config, 28) << " ===" << endl;

    string ip;
    cout << msg(config, 57);
    cin >> ip;

    int port = DEFAULT_PORT;
    cout << msg(config, 58);
    string portStr;
    cin >> portStr;
    if (!portStr.empty())
        port = atoi(portStr.c_str());
    if (port <= 0) port = DEFAULT_PORT;

    NetworkClient client;
    if (!client.connect(ip, port))
    {
        cout << "Connection failed!" << endl;
        waitForEnter();
        return;
    }
    cout << msg(config, 54) << endl;

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
            cout << state.players[state.gs.winner].name
                 << msg(config, 23) << endl;
            waitForEnter();
            break;
        }

        cout << msg(config, 13) << state.gs.currentCard << endl;
        cout << endl;

        for (int i = 0; i < (int)state.players.size(); i++)
        {
            if (i == myPlayerId)
            {
                cout << state.players[i].name << " (YOU)"
                     << ": " << state.players[i].hand.size() << " cards"
                     << endl;
                cout << msg(config, 5) << i + 1 << " ";
                for (int j = 0; j < (int)state.players[i].hand.size(); j++)
                    cout << j << ": " << state.players[i].hand[j] << "  ";
                cout << endl;
            }
            else
            {
                cout << state.players[i].name
                     << ": " << state.players[i].hand.size() << " cards"
                     << endl;
            }
        }
        cout << endl;

        if (state.gs.turn % state.players.size() != (size_t)myPlayerId)
        {
            cout << msg(config, 42) << endl;
            waitForEnter();
            continue;
        }

        if (state.gs.forceDraw)
        {
            cout << msg(config, 10) << " x" << state.gs.drawStack << endl;
            client.sendDraw(myPlayerId);
            waitForEnter();
            continue;
        }

        const SyncPlayer & me = state.players[myPlayerId];
        card currentCard = state.gs.currentCard;
        bool played = false;

        for (int j = 0; j < (int)me.hand.size(); j++)
        {
            if (canPlayCard(me.hand[j], currentCard))
            {
                played = true;
                break;
            }
        }

        if (!played)
        {
            cout << msg(config, 42) << endl;
            client.sendDraw(myPlayerId);
            waitForEnter();
            continue;
        }

        cout << msg(config, 14) << endl;
        cout << msg(config, 16);

        int idx;
        cin >> idx;

        if (idx >= 0 && idx < (int)me.hand.size())
        {
            card chosen = me.hand[idx];
            if (canPlayCard(chosen, currentCard))
            {
                string col;
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
    cout << "=== " << msg(config, 28) << " ===" << endl;
    cout << msg(config, 56);
    int choice;
    cin >> choice;

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
        cin >> choice;
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
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
                cout << msg(config, 31) << endl;
                waitForEnter();
                continue;
        }

        if (choice >= 1 && choice <= 4)
            waitForEnter();
    }
}
