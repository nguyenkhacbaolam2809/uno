#include "bot_easy.h"
#include "rng.h"

int EasyBotStrategy::pickCard(Player * self, const Card & current,
                              int, int, const int *, int, int)
{
    for (int i = 0; i < self->get_size(); i++)
    {
        Card c = self->peek(i);
        if (::canPlayCard(c, current))
            return i;
    }
    return -1;
}

CardColor EasyBotStrategy::pickColor(Player *) noexcept
{
    return static_cast<CardColor>(randomInt(1, 4));
}

bool EasyBotStrategy::shouldJumpIn(Player * self, const Card & target) noexcept
{
    for (int i = 0; i < self->get_size(); i++)
    {
        Card c = self->peek(i);
        if (::canJumpIn(c, target))
            return true;
    }
    return false;
}

std::unique_ptr<IBotStrategy> EasyBotStrategy::clone() const
{
    return std::make_unique<EasyBotStrategy>(*this);
}
