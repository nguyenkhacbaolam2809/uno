#include "card.h"
#include "player.h"

Player::Player()
{
    pname = "Player";
    ptype = PlayerType::Human;
    pdiff = BotDifficulty::Easy;
}

Player::Player(std::string name, PlayerType type, BotDifficulty diff)
{
    pname = name;
    ptype = type;
    pdiff = diff;
}

void Player::hand_add(const Card & temp_card)
{
    hand.push_back(temp_card);
}

Card Player::hand_remove(int pos) noexcept
{
    if (pos < 0 || pos >= static_cast<int>(hand.size()))
        return Card();
    Card c = hand[pos];
    hand.erase(hand.begin() + pos);
    return c;
}

int Player::get_size() const noexcept
{
    return static_cast<int>(hand.size());
}

Card Player::peek(int pos) const noexcept
{
    if (pos < 0 || pos >= get_size())
        return Card();
    return hand[pos];
}

std::string Player::getName() const noexcept { return pname; }
PlayerType Player::getType() const noexcept { return ptype; }
BotDifficulty Player::getDifficulty() const noexcept { return pdiff; }
void Player::setName(const std::string & n) { pname = n; }
void Player::setType(PlayerType t) { ptype = t; }
void Player::setDifficulty(BotDifficulty d) { pdiff = d; }
bool Player::isBot() const noexcept { return ptype == PlayerType::Bot; }
