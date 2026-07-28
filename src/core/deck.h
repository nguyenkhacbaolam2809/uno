#ifndef DECK_H
#define DECK_H

#include "card.h"
#include <vector>

class deck final {
public:
    deck();

    void create();
    void quick_shuffle() noexcept;
    card draw();
    int add_card(card c) noexcept;
    int get_size() const noexcept { return static_cast<int>(cards.size()); }
    void print_deck() const;

private:
    std::vector<card> cards;
};

#endif
