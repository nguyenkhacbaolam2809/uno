#ifndef DECK_H
#define DECK_H

#include "card.h"
#include <vector>

class deck final {
public:
    deck() noexcept;

    void create();
    void quick_shuffle() noexcept;
    card draw() noexcept;
    int add_card(const card & c) noexcept;
    int get_size() const noexcept { return static_cast<int>(cards.size()); }

private:
    std::vector<card> cards;
};

#endif
