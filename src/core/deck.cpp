#include "deck.h"
#include "rng.h"

Deck::Deck() noexcept
{
}

void Deck::create()
{
    cards.clear();

    for (int col = 1; col <= 4; col++)
    {
        cards.push_back(Card(0, static_cast<CardColor>(col)));
    }

    for (int num = 1; num <= CARD_REVERSE; num++)
    {
        for (int x = 0; x < 2; x++)
        {
            for (int col = 1; col <= 4; col++)
            {
                cards.push_back(Card(num, static_cast<CardColor>(col)));
            }
        }
    }

    for (int num = CARD_WILD; num <= CARD_WILD_DRAW_FOUR; num++)
    {
        for (int x = 0; x < 4; x++)
        {
            cards.push_back(Card(num, CardColor::Wild));
        }
    }
}

Card Deck::draw() noexcept
{
    if (cards.empty())
        return Card();
    Card c = cards.back();
    cards.pop_back();
    return c;
}

int Deck::add_card(const Card & c) noexcept
{
    cards.push_back(c);
    return 0;
}

void Deck::quick_shuffle() noexcept
{
    int n = static_cast<int>(cards.size());
    for (int i = n - 1; i > 0; i--)
    {
        int j = randomInt(0, i);
        Card temp = cards[i];
        cards[i] = cards[j];
        cards[j] = temp;
    }
}
