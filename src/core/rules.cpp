#include "rules.h"

int getStackValue(const card & c)
{
    if (c.number == 10) return 2;
    if (c.number == 14) return 4;
    return 0;
}

bool isStackCard(const card & c)
{
    return c.number == 10 || c.number == 14;
}

bool isActionCard(const card & c)
{
    return c.number >= 10 && c.number <= 14;
}

bool isSpecialCard(const card & c)
{
    return c.number == 13 || c.number == 14;
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
    if (c.number == 10 || c.number == 13 || c.number == 14) return false;
    return true;
}
