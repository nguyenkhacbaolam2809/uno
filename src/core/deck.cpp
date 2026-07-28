#include "deck.h"
#include <cstdlib>
#include <iostream>

deck::deck()
{
}

void deck::create()
{
    cards.clear();

    for (int col = 1; col <= 4; col++)
    {
        cards.push_back(card(0, static_cast<COLOR>(col)));
    }

    for (int num = 1; num <= CARD_REVERSE; num++)
    {
        for (int x = 0; x < 2; x++)
        {
            for (int col = 1; col <= 4; col++)
            {
                cards.push_back(card(num, static_cast<COLOR>(col)));
            }
        }
    }

    for (int num = CARD_WILD; num <= CARD_WILD_DRAW_FOUR; num++)
    {
        for (int x = 0; x < 4; x++)
        {
            cards.push_back(card(num, wild));
        }
    }
}

card deck::draw()
{
    if (cards.empty())
        return card();
    card c = cards.back();
    cards.pop_back();
    return c;
}

int deck::add_card(card c)
{
    cards.push_back(c);
    return 0;
}

void deck::quick_shuffle()
{
    int n = static_cast<int>(cards.size());
    for (int i = n - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        card temp = cards[i];
        cards[i] = cards[j];
        cards[j] = temp;
    }
}

void deck::print_deck()
{
    for (std::size_t i = 0; i < cards.size(); i++)
        std::cout << i << ": " << cards[i] << std::endl;
}

int deck::get_size() const
{
    return static_cast<int>(cards.size());
}
