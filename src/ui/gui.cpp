#include "gui.h"
#include "game_state.h"
#include "logger.h"
#include "audio_manager.h"
#include "colors.h"
#include <cstdlib>

using uno::SCREEN_W;
using uno::SCREEN_H;

Gui::Gui(const GameConfig & cfg) : config(cfg)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "UNO - Multiplayer Card Game");
    SetTargetFPS(60);

    AudioManager::instance().init();
}

Gui::~Gui()
{
    CloseWindow();
}

void Gui::run()
{
    LOG_INFO("%s", "GUI started, entering state machine");

    AppStateMachine sm(config);
    sm.run();

    LOG_INFO("%s", "GUI shutdown");
}
