#include "config.h"
#include "gui.h"
#include <cstdlib>
#include <ctime>

int main()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    GameConfig cfg;
    cfg.os = OS_UBUNTU;
    cfg.lang = LANG_ENGLISH;

    Gui gui(cfg);
    gui.run();

    return 0;
}
