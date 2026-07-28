#include "deck.h"
#include "rng.h"

deck::deck() noexcept
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

card deck::draw() noexcept
{
    if (cards.empty())
        return card();
    card c = cards.back();
    cards.pop_back();
    return c;
}

int deck::add_card(const card & c) noexcept
{
    cards.push_back(c);
    return 0;
}

void deck::quick_shuffle() noexcept
{
    int n = static_cast<int>(cards.size());
    for (int i = n - 1; i > 0; i--)
    {
        int j = randomInt(0, i);
        card temp = cards[i];
        cards[i] = cards[j];
        cards[j] = temp;
    }
}


