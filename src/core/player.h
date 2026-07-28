#ifndef PLAYER_H
#define PLAYER_H

#include "card.h"
#include <string>
#include <vector>

enum PlayerType : unsigned char { HUMAN, BOT };
enum BotDifficulty : unsigned char { D_EASY, D_NORMAL, D_HARD };

class player
{
    public:
        player();
        player(std::string name, PlayerType type = HUMAN, BotDifficulty diff = D_EASY);

        void hand_add(const card & temp);
        card hand_remove(int pos) noexcept;
        int get_size() const noexcept;
        card peek(int pos) const noexcept;

        std::string getName() const noexcept;
        PlayerType getType() const noexcept;
        BotDifficulty getDifficulty() const noexcept;
        void setName(const std::string & n);
        void setType(PlayerType t);
        void setDifficulty(BotDifficulty d);
        bool isBot() const noexcept;

    private:
        std::vector<card> hand;
        std::string pname;
        PlayerType ptype;
        BotDifficulty pdiff;
};

#endif
