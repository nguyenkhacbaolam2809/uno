#include "rules.h"

int getStackValue(const card & c)
{
    if (c.number == CARD_DRAW_TWO) return 2;
    if (c.number == CARD_WILD_DRAW_FOUR) return 4;
    return 0;
}

bool isStackCard(const card & c)
{
    return c.number == CARD_DRAW_TWO || c.number == CARD_WILD_DRAW_FOUR;
}

bool isActionCard(const card & c)
{
    return c.number >= CARD_DRAW_TWO && c.number <= CARD_WILD_DRAW_FOUR;
}

bool isSpecialCard(const card & c)
{
    return c.number == CARD_WILD || c.number == CARD_WILD_DRAW_FOUR;
}

bool canPlayCard(const card & played, const card & current)
{
    if (played.color == wild) return true;
    if (played.number == current.number) return true;
    if (played.color == current.color) return true;
    return false;
}

bool canJumpIn(const card & played, const card & current)
{
    return played.number == current.number && played.color == current.color;
}

bool isLegalLastCard(const card & c)
{
    if (c.number == CARD_DRAW_TWO ||
        c.number == CARD_WILD ||
        c.number == CARD_WILD_DRAW_FOUR) return false;
    return true;
}
