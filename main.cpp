#include "config.h"
#include "console_ui.h"
#include <cstdlib>
#include <ctime>

int main()
{
    srand(time(NULL));

    GameConfig config = promptConfig();

    ConsoleUI ui(config);
    ui.run();

    return 0;
}
