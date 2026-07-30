#ifndef PLAYER_H
#define PLAYER_H

#include "card.h"
#include <string>
#include <vector>

enum class PlayerType : unsigned char { Human, Bot };
enum class BotDifficulty : unsigned char { Easy, Normal, Hard };

class Player
{
    public:
        Player();
        Player(std::string name, PlayerType type = PlayerType::Human,
               BotDifficulty diff = BotDifficulty::Easy);

        void hand_add(const Card & temp);
        Card hand_remove(int pos) noexcept;
        [[nodiscard]] int get_size() const noexcept;
        Card peek(int pos) const noexcept;

        [[nodiscard]] std::string getName() const noexcept;
        [[nodiscard]] PlayerType getType() const noexcept;
        [[nodiscard]] BotDifficulty getDifficulty() const noexcept;
        void setName(const std::string & n);
        void setType(PlayerType t);
        void setDifficulty(BotDifficulty d);
        [[nodiscard]] bool isBot() const noexcept;

    private:
        std::vector<Card> hand;
        std::string pname;
        PlayerType ptype;
        BotDifficulty pdiff;
};

#endif
