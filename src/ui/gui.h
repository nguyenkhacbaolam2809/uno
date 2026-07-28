#ifndef GUI_H
#define GUI_H

#include "config.h"

class Gui {
public:
    explicit Gui(const GameConfig & cfg);
    ~Gui();

    void run();

private:
    GameConfig config;
};

#endif
