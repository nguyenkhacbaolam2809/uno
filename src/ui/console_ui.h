#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include "config.h"
#include "game_engine.h"
#include "player.h"
#include "network_server.h"
#include "network_client.h"
#include <string>

class ConsoleUI
{
public:
    ConsoleUI(const GameConfig & cfg);
    ~ConsoleUI();

    void run();

private:
    GameConfig config;
    bool vietRules;

    void showIntro();
    bool askVietnameseRules();
    void showMainMenu();

    void modeSinglePlayer();
    void modeLocalMultiplayer();
    void modeMixed();
    void modeLanServer();
    void modeLanClient();
    void modeLan();

    void waitForEnter();
    void showTurnHeader(player & p, const GameEngine & engine);
    void showGameStatus(const GameEngine & engine);
    void handleHumanTurn(GameEngine & engine, int playerIdx);
    void handleBotTurn(GameEngine & engine, int playerIdx);
    int pickCardFromHand(player & p, const card & current, bool forceDraw);
    std::string pickColor();
    void processGameLoop(GameEngine & engine);
};

#endif
