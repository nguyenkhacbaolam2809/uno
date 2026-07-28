#include "card.h"

card::card() : number(0), color(wild)
{
}

card::card(int num, COLOR col) : number(num), color(col)
{
}

bool card::operator==(card const & other) const
{
    return number == other.number || color == other.color || color == wild || other.color == wild;
}

bool card::operator!=(card const & other) const
{
    return !(*this == other);
}

std::ostream & operator<<(std::ostream & out, card const & c)
{
    out << "Number:";
    switch (c.number)
    {
        case CARD_DRAW_TWO:       out << "DRAW-2";     break;
        case CARD_SKIP:           out << "SKIP";       break;
        case CARD_REVERSE:        out << "REVERSE";    break;
        case CARD_WILD:           out << "WILD";       break;
        case CARD_WILD_DRAW_FOUR: out << "DRAW-4-WILD"; break;
        default: out << (int)c.number; break;
    }

    out << "   Color:";
    switch (c.color)
    {
        case wild:   out << "wild";   break;
        case red:    out << "red";    break;
        case green:  out << "green";  break;
        case blue:   out << "blue";   break;
        case yellow: out << "yellow"; break;
        default:     out << "N/A";    break;
    }
    return out;
}
