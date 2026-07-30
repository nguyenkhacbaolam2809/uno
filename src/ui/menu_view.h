#ifndef MENU_VIEW_H
#define MENU_VIEW_H

#include "raylib.h"
#include "config.h"
#include "colors.h"
#include "player.h"
#include <string>
#include <vector>
#include <functional>

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

enum class MenuPhase {
    Main,
    Difficulty,
    LocalSetup,
    MixedSetup,
    Lan,
    ColorPicker,
    Done
};

class MenuView {
public:
    MenuView(const GameConfig & cfg);

    // Blocking: runs its own render loop (old-style)
    MenuResult show();

    // Non-blocking: one frame at a time
    bool step();
    void drawCurrentMenu();
    MenuResult getStepResult() const { return m_stepResult; }
    bool isStepDone() const { return m_phase == MenuPhase::Done; }

    void resetTo(const MenuPhase & phase);

private:
    GameConfig config;

    struct Button {
        Rectangle rect;
        std::string text;
        Color color;
        bool hovered;
    };

    MenuPhase m_phase{MenuPhase::Main};
    MenuResult m_stepResult;
    bool m_stringEditing{false};
    int m_editingField{0};

    // Persistent state for each sub-menu
    int m_diffChoice{1};
    char m_nameBuf[64]{"Player"};
    int m_nameLen{6};
    int m_botCount{1};
    int m_humanCount{2};
    bool m_vietRules{false};
    char m_ipBuf[64]{"127.0.0.1"};
    int m_ipLen{9};
    char m_portBuf[8]{"8888"};
    int m_portLen{4};
    bool m_editingIp{false};
    bool m_editingPort{false};

    void stepMain();
    void stepDifficulty();
    void stepLocalSetup();
    void stepMixedSetup();
    void stepLan();
    void stepColorPicker();

    void drawMain();
    void drawDifficulty();
    void drawLocalSetup();
    void drawMixedSetup();
    void drawLan();
    void drawColorPicker();

    // Legacy blocking wrappers (call step* in a loop)
    MenuResult runBlocking(MenuPhase phase);
    void drawTitle(const char * text, int y, int fontSize, Color color);
    Button makeButton(int x, int y, int w, int h, const std::string & text, Color color);
    bool drawButton(Button & btn);
    void drawTextCentered(const char * text, int y, int fontSize, Color color);
    const char * msg(int id) const;
};

#endif
