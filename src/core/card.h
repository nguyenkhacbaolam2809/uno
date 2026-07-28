#ifndef CARD_H
#define CARD_H

#include <ostream>

enum COLOR { wild, red, green, blue, yellow};

constexpr int CARD_DRAW_TWO = 10;
constexpr int CARD_SKIP    = 11;
constexpr int CARD_REVERSE = 12;
constexpr int CARD_WILD    = 13;
constexpr int CARD_WILD_DRAW_FOUR = 14;
constexpr int DECK_SIZE    = 108;
constexpr int HAND_SIZE    = 7;
constexpr int MAX_PLAYERS  = 5;

class card
{
    public:
        int number;
        COLOR color;

        bool operator==(card const & other) const;
        bool operator!=(card const & other) const;

        card();
        card(int num, COLOR col);
};

std::ostream & operator<<(std::ostream & out, card const & temp_card);

#endif
