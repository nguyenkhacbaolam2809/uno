#include "bot_medium.h"

int MediumBotStrategy::pickCard(player * self, const card & current,
                                int, int, const int *, int, int)
{
    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (c.number == current.number && c.color != wild)
            return i;
    }

    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (c.color == current.color && c.color != wild && !::isActionCard(c))
            return i;
    }

    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (::canPlayCard(c, current) && c.color != wild)
            return i;
    }

    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (c.number == 13)
            return i;
    }

    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (::canPlayCard(c, current))
            return i;
    }

    return -1;
}

COLOR MediumBotStrategy::pickColor(player * self)
{
    int counts[4] = {0};
    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (c.color >= 1 && c.color <= 4)
            counts[c.color - 1]++;
    }

    int bestCol = 1;
    int bestCount = counts[0];
    for (int i = 1; i < 4; i++)
    {
        if (counts[i] > bestCount)
        {
            bestCount = counts[i];
            bestCol = i + 1;
        }
    }
    return static_cast<COLOR>(bestCol);
}

bool MediumBotStrategy::shouldJumpIn(player * self, const card & target)
{
    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (::canJumpIn(c, target))
            return true;
    }
    return false;
}

IBotStrategy * MediumBotStrategy::clone() const
{
    return new MediumBotStrategy(*this);
}
