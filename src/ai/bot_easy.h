#ifndef BOT_EASY_H
#define BOT_EASY_H

#include "ibot_strategy.h"
#include "rules.h"

class EasyBotStrategy : public IBotStrategy
{
public:
    int pickCard(player * self, const card & current,
                 int direction, int selfIdx,
                 const int * opponentSizes, int opponentCount,
                 int drawStack) override;

    COLOR pickColor(player * self) override;

    bool shouldJumpIn(player * self, const card & target) override;

    std::unique_ptr<IBotStrategy> clone() const override;
};

#endif
