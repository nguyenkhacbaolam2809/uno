#include "card.h"

card::card() noexcept : number(0), color(wild)
{
}

card::card(int num, COLOR col) noexcept : number(num), color(col)
{
}

bool card::operator==(card const & other) const noexcept
{
    return number == other.number || color == other.color
        || color == wild || other.color == wild;
}

bool card::operator!=(card const & other) const noexcept
{
    return !(*this == other);
}


