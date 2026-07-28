#ifndef PLAYER_H
#define PLAYER_H

#include "card.h"
#include <string>
#include <vector>

enum PlayerType { HUMAN, BOT };
enum BotDifficulty { D_EASY, D_NORMAL, D_HARD };

class player
{
    public:
        player();
        player(std::string name, PlayerType type = HUMAN, BotDifficulty diff = D_EASY);

        void hand_add(card temp);
        card hand_remove(int pos);
        void print() const;
        int get_size() const;
        card peek(int pos) const;

        std::string getName() const;
        PlayerType getType() const;
        BotDifficulty getDifficulty() const;
        void setName(const std::string & n);
        void setType(PlayerType t);
        void setDifficulty(BotDifficulty d);
        bool isBot() const;

    private:
        std::vector<card> hand;
        std::string pname;
        PlayerType ptype;
        BotDifficulty pdiff;
};

#endif
