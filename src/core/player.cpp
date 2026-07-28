#include "card.h"
#include "player.h"
#include <iostream>

player::player()
{
    pname = "Player";
    ptype = HUMAN;
    pdiff = D_EASY;
}

player::player(std::string name, PlayerType type, BotDifficulty diff)
{
    pname = name;
    ptype = type;
    pdiff = diff;
}

void player::hand_add(const card & temp_card)
{
    hand.push_back(temp_card);
}

card player::hand_remove(int pos)
{
    if (pos < 0 || pos >= static_cast<int>(hand.size()))
        return card();
    card c = hand[pos];
    hand.erase(hand.begin() + pos);
    return c;
}

void player::print() const
{
    for (std::size_t i = 0; i < hand.size(); i++)
        std::cout << i << ":  " << hand[i] << std::endl;
}

int player::get_size() const
{
    return static_cast<int>(hand.size());
}

card player::peek(int pos) const
{
    if (pos < 0 || pos >= get_size())
        return card();
    return hand[pos];
}

std::string player::getName() const { return pname; }
PlayerType player::getType() const { return ptype; }
BotDifficulty player::getDifficulty() const { return pdiff; }
void player::setName(const std::string & n) { pname = n; }
void player::setType(PlayerType t) { ptype = t; }
void player::setDifficulty(BotDifficulty d) { pdiff = d; }
bool player::isBot() const { return ptype == BOT; }
