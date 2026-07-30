#include "card.h"

Card::Card() noexcept : number(0), color(CardColor::Wild)
{
}

Card::Card(int num, CardColor col) noexcept : number(num), color(col)
{
}

bool Card::operator==(Card const & other) const noexcept
{
    return number == other.number && color == other.color;
}

bool Card::operator!=(Card const & other) const noexcept
{
    return !(*this == other);
}
