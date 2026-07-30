#ifndef BOT_HARD_H
#define BOT_HARD_H

#include "ibot_strategy.h"
#include "rules.h"

class HardBotStrategy : public IBotStrategy
{
public:
    HardBotStrategy();
    HardBotStrategy(const HardBotStrategy & other);

    int pickCard(Player * self, const Card & current,
                 int direction, int selfIdx,
                 const int * opponentSizes, int opponentCount,
                 int drawStack) override;

    CardColor pickColor(Player * self) noexcept override;

    bool shouldJumpIn(Player * self, const Card & target) noexcept override;

    std::unique_ptr<IBotStrategy> clone() const override;

private:
    int prevOppSizes[5];
    int forcedColors[4];
    bool initialized;

    void detectDrewCards(const int * opponentSizes, int opponentCount,
                         const Card & current);
    int findStackDefense(Player * self, const Card & current, int drawStack) const;
    int findThreatBlock(Player * self, const Card & current,
                        int direction, int selfIdx,
                        const int * opponentSizes, int opponentCount) const;
};

#endif
