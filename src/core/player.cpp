#include "card.h"
#include "player.h"

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

card player::hand_remove(int pos) noexcept
{
    if (pos < 0 || pos >= static_cast<int>(hand.size()))
        return card();
    card c = hand[pos];
    hand.erase(hand.begin() + pos);
    return c;
}

int player::get_size() const noexcept
{
    return static_cast<int>(hand.size());
}

card player::peek(int pos) const noexcept
{
    if (pos < 0 || pos >= get_size())
        return card();
    return hand[pos];
}

std::string player::getName() const noexcept { return pname; }
PlayerType player::getType() const noexcept { return ptype; }
BotDifficulty player::getDifficulty() const noexcept { return pdiff; }
void player::setName(const std::string & n) { pname = n; }
void player::setType(PlayerType t) { ptype = t; }
void player::setDifficulty(BotDifficulty d) { pdiff = d; }
bool player::isBot() const noexcept { return ptype == BOT; }
