#ifndef IBOT_STRATEGY_H
#define IBOT_STRATEGY_H

#include "card.h"
#include "player.h"

class IBotStrategy
{
public:
    virtual ~IBotStrategy() = default;

    virtual int pickCard(player * self, const card & current,
                         int direction, int selfIdx,
                         const int * opponentSizes, int opponentCount,
                         int drawStack) = 0;

    virtual COLOR pickColor(player * self) = 0;

    virtual bool shouldJumpIn(player * self, const card & target) = 0;

    virtual IBotStrategy * clone() const = 0;
};

#endif
