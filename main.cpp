#include "config.h"
#include "gui.h"
#include "console_ui.h"
#include <cstring>

int main(int argc, char * argv[])
{
    GameConfig cfg;
    cfg.lang = Language::English;

    if (argc > 1 && std::strcmp(argv[1], "--cli") == 0)
        return runConsoleMode(cfg);

    Gui gui(cfg);
    gui.run();

    return 0;
}
