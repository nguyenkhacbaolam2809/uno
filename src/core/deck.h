#ifndef DECK_H
#define DECK_H

#include "card.h"
#include <vector>

class Deck final {
public:
    Deck() noexcept;

    void create();
    void quick_shuffle() noexcept;
    Card draw() noexcept;
    int add_card(const Card & c) noexcept;
    [[nodiscard]] int get_size() const noexcept { return static_cast<int>(cards.size()); }

private:
    std::vector<Card> cards;
};

#endif
