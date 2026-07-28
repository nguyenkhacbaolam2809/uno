#ifndef DECK_H
#define DECK_H

#include "card.h"
#include <vector>

class deck {
public:
    deck();

    void create();
    void quick_shuffle();
    card draw();
    int add_card(card c);
    int get_size() const;
    void print_deck();

private:
    std::vector<card> cards;
};

#endif
