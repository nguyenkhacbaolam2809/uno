#ifndef BOT_HARD_H
#define BOT_HARD_H

#include "ibot_strategy.h"
#include "rules.h"

class HardBotStrategy : public IBotStrategy
{
public:
    HardBotStrategy();
    HardBotStrategy(const HardBotStrategy & other);

    int pickCard(player * self, const card & current,
                 int direction, int selfIdx,
                 const int * opponentSizes, int opponentCount,
                 int drawStack) override;

    COLOR pickColor(player * self) override;

    bool shouldJumpIn(player * self, const card & target) override;

    IBotStrategy * clone() const override;

private:
    int prevOppSizes[5];
    int forcedColors[4];
    bool initialized;

    void detectDrewCards(const int * opponentSizes, int opponentCount,
                         const card & current);
    int findStackDefense(player * self, const card & current, int drawStack) const;
    int findThreatBlock(player * self, const card & current,
                        int direction, int selfIdx,
                        const int * opponentSizes, int opponentCount) const;
    int findPointMinimization(player * self, const card & current) const;
};

#endif
