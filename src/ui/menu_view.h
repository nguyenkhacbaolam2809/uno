#ifndef MENU_VIEW_H
#define MENU_VIEW_H

#include "raylib.h"
#include "config.h"
#include "colors.h"
#include <string>
#include <vector>
#include <functional>

struct MenuResult {
    int action; // 1=Single, 2=Local, 3=Mixed, 4=LAN host, 5=LAN join, -1=exit
    int botDifficulty;
    int numHumans;
    int numBots;
    bool vietRules;
    std::string playerName;
    std::string serverIp;
    int port;
    bool confirmed;
};

class MenuView {
public:
    MenuView(const GameConfig & cfg);

    MenuResult show();

private:
    GameConfig config;
    Font font;

    struct Button {
        Rectangle rect;
        std::string text;
        Color color;
        Color hoverColor;
        bool hovered;
    };

    std::vector<Button> mainButtons;

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
