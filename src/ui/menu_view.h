#ifndef MENU_VIEW_H
#define MENU_VIEW_H

#include "raylib.h"
#include "config.h"
#include "colors.h"
#include "player.h"
#include <string>
#include <vector>

struct MenuResult {
    int action{0};
    int botDifficulty{static_cast<int>(BotDifficulty::Easy)};
    int numHumans{2};
    int numBots{0};
    bool vietRules{false};
    std::string playerName{"Player"};
    std::string serverIp{"127.0.0.1"};
    int port{8080};
    bool confirmed{false};
};

class MenuView {
public:
    MenuView(const GameConfig & cfg);

    MenuResult show();

private:
    GameConfig config;

    struct Button {
        Rectangle rect;
        std::string text;
        Color color;
        bool hovered;
    };

    MenuResult showMainMenu();
    MenuResult showDifficultySelect();
    MenuResult showLocalSetup();
    MenuResult showMixedSetup();
    MenuResult showLanMenu();
    MenuResult showColorPicker();

    void drawTitle(const char * text, int y, int fontSize, Color color);
    Button makeButton(int x, int y, int w, int h, const std::string & text, Color color);
    bool drawButton(Button & btn);
    void drawTextCentered(const char * text, int y, int fontSize, Color color);
    const char * msg(int id) const;
};

#endif
