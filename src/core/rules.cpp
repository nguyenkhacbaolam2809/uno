#include "rules.h"
#include "player.h"

int getStackValue(const Card & c)
{
    if (c.number == CARD_DRAW_TWO) return 2;
    if (c.number == CARD_WILD_DRAW_FOUR) return 4;
    return 0;
}

bool isStackCard(const Card & c)
{
    return c.number == CARD_DRAW_TWO || c.number == CARD_WILD_DRAW_FOUR;
}

bool isActionCard(const Card & c)
{
    return c.number >= CARD_DRAW_TWO && c.number <= CARD_WILD_DRAW_FOUR;
}

bool isSpecialCard(const Card & c)
{
    return c.number == CARD_WILD || c.number == CARD_WILD_DRAW_FOUR;
}

bool canPlayCard(const Card & played, const Card & current)
{
    if (played.color == CardColor::Wild) return true;
    if (played.number == current.number) return true;
    if (played.color == current.color) return true;
    return false;
}

bool canJumpIn(const Card & played, const Card & current)
{
    return played.number == current.number && played.color == current.color;
}

bool isLegalLastCard(const Card & c)
{
    if (c.number == CARD_DRAW_TWO ||
        c.number == CARD_WILD ||
        c.number == CARD_WILD_DRAW_FOUR) return false;
    return true;
}

bool canPlayWildDrawFour(const Card & played, const Card & current, const Player & self)
{
    if (played.number != CARD_WILD_DRAW_FOUR) return true;

    for (int i = 0; i < self.get_size(); i++)
    {
        Card c = self.peek(i);
        if (c.number == played.number && c.color == played.color)
            continue;
        if (c.color == current.color && c.color != CardColor::Wild)
            return false;
    }
    return true;
}
