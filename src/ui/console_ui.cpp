#include "console_ui.h"
#include "bot_factory.h"
#include "rules.h"
#include "rng.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

static void enableANSI()
{
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode))
    {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }
#endif
}

static const char * colorEscape(CardColor c)
{
    switch (c)
    {
        case CardColor::Red:    return "\033[31m";
        case CardColor::Green:  return "\033[32m";
        case CardColor::Yellow: return "\033[33m";
        case CardColor::Blue:   return "\033[34m";
        default:                return "\033[37m";
    }
}

static const char * colorName(CardColor c)
{
    switch (c)
    {
        case CardColor::Red:    return "Red";
        case CardColor::Green:  return "Green";
        case CardColor::Blue:   return "Blue";
        case CardColor::Yellow: return "Yellow";
        default:                return "Wild";
    }
}

static const char * cardLabel(const Card & c)
{
    switch (c.number)
    {
        case CARD_DRAW_TWO:       return "+2";
        case CARD_SKIP:           return "Skip";
        case CARD_REVERSE:        return "Reverse";
        case CARD_WILD:           return "Wild";
        case CARD_WILD_DRAW_FOUR: return "+4";
        default: {
            static char buf[8];
            std::snprintf(buf, sizeof(buf), "%d", c.number);
            return buf;
        }
    }
}

static void printCard(const Card & c, bool withIndex = false, int idx = -1)
{
    if (withIndex && idx >= 0)
        std::cout << idx + 1 << ". ";

    if (c.color == CardColor::Wild)
        std::cout << "\033[37m" << cardLabel(c) << "\033[0m";
    else
        std::cout << colorEscape(c.color) << cardLabel(c) << " " << colorName(c.color) << "\033[0m";
}

static void clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void renderState(const GameEngine & engine, int localPlayerId)
{
    clearScreen();

    std::cout << "\n========== UNO! ==========\n" << std::endl;

    const Card & current = engine.getCurrentCard();
    std::cout << "Current card: ";
    printCard(current);
    std::cout << std::endl;

    if (engine.isForceDraw())
        std::cout << "FORCED DRAW: " << engine.getDrawStack() << " cards!" << std::endl;

    std::cout << "Direction: " << (engine.getDirection() == 1 ? "Clockwise" : "Counter-Clockwise") << std::endl;
    std::cout << std::endl;

    int n = engine.getPlayerCount();
    int currentTurn = engine.getCurrentTurn() % n;

    for (int i = 0; i < n; i++)
    {
        const Player * p = engine.getPlayer(i);
        bool isCurrent = (i == currentTurn);

        std::cout << (isCurrent ? ">> " : "   ");

        if (i == localPlayerId)
            std::cout << "YOU";
        else
            std::cout << p->getName();

        std::cout << " [" << p->get_size() << " cards]";
        if (p->get_size() == 1)
            std::cout << " UNO!";
        std::cout << std::endl;
    }

    std::cout << std::endl;

    if (engine.getCurrentTurn() % n == localPlayerId)
    {
        const Player * me = engine.getPlayer(localPlayerId);
        std::cout << "Your hand:" << std::endl;
        for (int i = 0; i < me->get_size(); i++)
        {
            Card c = me->peek(i);
            bool playable = canPlayCard(c, current);
            std::cout << "  ";
            printCard(c, true, i);
            if (playable)
                std::cout << " (playable)";
            std::cout << std::endl;
        }
    }
}

static void botTurnSleep()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

static CardColor chooseColor()
{
    int choice = 0;
    while (choice < 1 || choice > 4)
    {
        std::cout << "\nChoose color: 1=Red 2=Green 3=Blue 4=Yellow: ";
        std::string input;
        std::getline(std::cin, input);
        choice = std::atoi(input.c_str());
        if (choice < 1 || choice > 4)
            std::cout << "Invalid choice!" << std::endl;
    }
    return static_cast<CardColor>(choice);
}

int runConsoleMode(const GameConfig & cfg)
{
    enableANSI();

    int numHumans = 1;
    int numBots = 1;
    int diff = 1;
    bool vietRules = false;

    std::cout << "=== UNO - Console Mode ===" << std::endl;

    std::cout << "Number of human players (1-4): ";
    std::string input;
    std::getline(std::cin, input);
    numHumans = std::max(1, std::min(4, std::atoi(input.c_str())));

    std::cout << "Number of bot players (0-4): ";
    std::getline(std::cin, input);
    numBots = std::max(0, std::min(4, std::atoi(input.c_str())));

    if (numBots > 0)
    {
        std::cout << "Bot difficulty: 1=Easy 2=Normal 3=Hard: ";
        std::getline(std::cin, input);
        diff = std::max(1, std::min(3, std::atoi(input.c_str())));
    }

    std::cout << "Vietnamese rules? (y/n): ";
    std::getline(std::cin, input);
    vietRules = (input == "y" || input == "Y");

    int total = std::max(numHumans + numBots, 2);
    int localPlayerId = 0;

    GameEngine engine(cfg, vietRules);
    engine.init(total);

    for (int i = 0; i < total; i++)
    {
        std::string name;
        if (i < numHumans)
        {
            if (numHumans == 1)
                name = "Player";
            else
            {
                std::cout << "Name for player " << (i + 1) << ": ";
                std::getline(std::cin, name);
                if (name.empty()) name = "Player " + std::to_string(i + 1);
            }
            if (i == 0) localPlayerId = i;
            engine.addPlayer(name, PlayerType::Human, 0);
        }
        else
        {
            name = "Bot " + std::to_string(i);
            engine.addPlayer(name, PlayerType::Bot, diff - 1);
        }
    }

    engine.start();

    while (!engine.isGameOver())
    {
        int n = engine.getPlayerCount();
        int currentTurn = engine.getCurrentTurn() % n;
        const Player * p = engine.getPlayer(currentTurn);

        renderState(engine, localPlayerId);

        if (p->isBot())
        {
            BotActionResult act = engine.executeBotTurn(currentTurn);
            std::cout << "\n" << p->getName() << " is thinking..." << std::endl;
            botTurnSleep();

            if (act.action == BotAction::PlayCard || act.action == BotAction::StackCard)
            {
                Card c = p->peek(act.cardIdx);
                std::cout << p->getName() << " plays ";
                printCard(c);
                if (c.color == CardColor::Wild)
                    std::cout << " (chosen " << colorName(act.chosenColor) << ")";
                std::cout << std::endl;
                engine.playCard(currentTurn, act.cardIdx, act.chosenColor);
            }
            else
            {
                engine.drawCard(currentTurn);
                std::cout << p->getName() << " draws a card." << std::endl;
            }
            engine.nextTurn();
            botTurnSleep();
        }
        else
        {
            bool turnDone = false;
            bool drewCard = false;

            while (!turnDone && !engine.isGameOver())
            {
                const Player * me = engine.getPlayer(localPlayerId);

                std::cout << "\nYour turn!" << std::endl;
                if (engine.isForceDraw() && !drewCard)
                {
                    std::cout << "You must draw " << engine.getDrawStack()
                              << " cards (or play a matching stack card)." << std::endl;
                }

                std::cout << "Enter command: (1-" << me->get_size() << "=play card, d=draw, u=uno, q=quit): ";
                std::string cmd;
                std::getline(std::cin, cmd);

                if (cmd == "q") return 0;
                if (cmd == "d")
                {
                    if (!drewCard || engine.isForceDraw())
                    {
                        engine.drawCard(localPlayerId);
                        drewCard = true;
                        if (!engine.isForceDraw())
                        {
                            engine.nextTurn();
                            turnDone = true;
                        }
                        else
                        {
                            engine.nextTurn();
                            turnDone = true;
                        }
                    }
                    else
                    {
                        std::cout << "You already drew this turn!" << std::endl;
                    }
                    continue;
                }
                if (cmd == "u")
                {
                    engine.callUno(localPlayerId);
                    std::cout << "UNO called!" << std::endl;
                    continue;
                }

                int cardIdx = std::atoi(cmd.c_str()) - 1;
                if (cardIdx >= 0 && cardIdx < me->get_size())
                {
                    Card chosen = me->peek(cardIdx);
                    if (engine.validatePlay(localPlayerId, cardIdx))
                    {
                        CardColor col = chosen.color;
                        if (col == CardColor::Wild)
                            col = chooseColor();
                        engine.playCard(localPlayerId, cardIdx, col);
                        turnDone = true;
                        drewCard = false;
                        engine.nextTurn();
                    }
                    else
                    {
                        std::cout << "Cannot play that card!" << std::endl;
                    }
                }
                else
                {
                    std::cout << "Invalid card number!" << std::endl;
                }
            }
        }
    }

    renderState(engine, localPlayerId);
    int winner = engine.getWinner();
    if (winner >= 0)
    {
        const Player * wp = engine.getPlayer(winner);
        std::cout << "\n\n*** " << wp->getName() << " wins! ***" << std::endl;
    }

    std::cout << "\nPress Enter to exit...";
    std::getline(std::cin, input);
    return 0;
}
