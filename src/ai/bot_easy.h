#ifndef BOT_EASY_H
#define BOT_EASY_H

#include "ibot_strategy.h"
#include "rules.h"

class EasyBotStrategy : public IBotStrategy
{
public:
    int pickCard(Player * self, const Card & current,
                 int direction, int selfIdx,
                 const int * opponentSizes, int opponentCount,
                 int drawStack) override;

    CardColor pickColor(Player * self) noexcept override;

    bool shouldJumpIn(Player * self, const Card & target) noexcept override;

    std::unique_ptr<IBotStrategy> clone() const override;
};

#endif
