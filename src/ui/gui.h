#ifndef GUI_H
#define GUI_H

#include "raylib.h"
#include "config.h"
#include "game_engine.h"
#include "game_view.h"
#include "menu_view.h"
#include <memory>

class Gui {
public:
    Gui(const GameConfig & cfg);
    ~Gui();

    void run();

private:
    GameConfig config;
    bool vietRules;
    bool running;

    std::unique_ptr<GameView> gameView;
    std::unique_ptr<MenuView> menuView;

    void runSinglePlayer(const MenuResult & cfg);
    void runLocalMultiplayer(const MenuResult & cfg);
    void runMixed(const MenuResult & cfg);
    void runLanServer(const MenuResult & cfg);
    void runLanClient(const MenuResult & cfg);

    void processLocalTurn(GameEngine & engine, int localPlayerId);

    void raylibTraceLog(int msgType, const char * text, va_list args);
};

#endif
