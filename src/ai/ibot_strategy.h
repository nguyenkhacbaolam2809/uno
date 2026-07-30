#ifndef IBOT_STRATEGY_H
#define IBOT_STRATEGY_H

#include "card.h"
#include "player.h"
#include <memory>

class IBotStrategy
{
public:
    virtual ~IBotStrategy() = default;

    virtual int pickCard(Player * self, const Card & current,
                         int direction, int selfIdx,
                         const int * opponentSizes, int opponentCount,
                         int drawStack) = 0;

    virtual CardColor pickColor(Player * self) = 0;

    virtual bool shouldJumpIn(Player * self, const Card & target) = 0;

    virtual std::unique_ptr<IBotStrategy> clone() const = 0;
};

#endif
