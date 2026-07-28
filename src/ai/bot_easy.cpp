#include "bot_easy.h"
#include "rng.h"

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

COLOR EasyBotStrategy::pickColor(player *) noexcept
{
    return static_cast<COLOR>(randomInt(1, 4));
}

bool EasyBotStrategy::shouldJumpIn(player * self, const card & target) noexcept
{
    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (::canJumpIn(c, target))
            return true;
    }
    return false;
}

std::unique_ptr<IBotStrategy> EasyBotStrategy::clone() const
{
    return std::make_unique<EasyBotStrategy>(*this);
}
