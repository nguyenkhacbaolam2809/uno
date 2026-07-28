#include "config.h"
#include "gui.h"

int main()
{

    GameConfig cfg;
    cfg.os = OS_UBUNTU;
    cfg.lang = LANG_ENGLISH;

    Gui gui(cfg);
    gui.run();

    return 0;
}
