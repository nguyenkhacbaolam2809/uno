#include "bot_easy.h"
#include <cstdlib>
using namespace std;

int EasyBotStrategy::pickCard(player * self, const card & current,
                              int, int, const int *, int, int)
{
    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (::canPlayCard(c, current))
            return i;
    }
    return -1;
}

COLOR EasyBotStrategy::pickColor(player *)
{
    int r = rand() % 4;
    return static_cast<COLOR>(r + 1);
}

bool EasyBotStrategy::shouldJumpIn(player * self, const card & target)
{
    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (::canJumpIn(c, target))
            return true;
    }
    return false;
}

IBotStrategy * EasyBotStrategy::clone() const
{
    return new EasyBotStrategy(*this);
}
