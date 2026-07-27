#ifndef PLAYER_H
#define PLAYER_H

#include "card.h"
#include <string>

enum PlayerType { HUMAN, BOT };
enum BotDifficulty { D_EASY, D_NORMAL, D_HARD };

class player
{
    public:
        player();
        player(std::string name, PlayerType type = HUMAN, BotDifficulty diff = D_EASY);
        player(const player & other);
        const player & operator= (const player & other);
        ~player();
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
        class card_elem
        {
          public:
            card_elem() { next = NULL; }
            card data;
            card_elem * next;
        };

        card_elem * head;
        int size;
        std::string pname;
        PlayerType ptype;
        BotDifficulty pdiff;
        void copy(const player & other);
        void clear();
};

#endif
