#include "bot_hard.h"
#include <cstring>

HardBotStrategy::HardBotStrategy()
{
    for (int i = 0; i < 5; i++) prevOppSizes[i] = -1;
    for (int i = 0; i < 4; i++) forcedColors[i] = 0;
    initialized = false;
}

HardBotStrategy::HardBotStrategy(const HardBotStrategy & other)
{
    for (int i = 0; i < 5; i++) prevOppSizes[i] = other.prevOppSizes[i];
    for (int i = 0; i < 4; i++) forcedColors[i] = other.forcedColors[i];
    initialized = other.initialized;
}

void HardBotStrategy::detectDrewCards(const int * opponentSizes, int opponentCount,
                                      const card & current)
{
    if (!initialized)
    {
        for (int i = 0; i < opponentCount; i++)
            prevOppSizes[i] = opponentSizes[i];
        initialized = true;
        return;
    }

    for (int i = 0; i < opponentCount; i++)
    {
        if (opponentSizes[i] > prevOppSizes[i] && current.color >= 1 && current.color <= 4)
        {
            forcedColors[current.color - 1]++;
        }
        prevOppSizes[i] = opponentSizes[i];
    }
}

int HardBotStrategy::findStackDefense(player * self, const card & current,
                                      int drawStack) const
{
    if (drawStack <= 0) return -1;

    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (::isStackCard(c) && ::canPlayCard(c, current))
            return i;
    }
    return -1;
}

int HardBotStrategy::findThreatBlock(player * self, const card & current,
                                     int direction, int selfIdx,
                                     const int * opponentSizes, int opponentCount) const
{
    int nextIdx = (selfIdx + direction + opponentCount * 2) % opponentCount;

    if (opponentSizes[nextIdx] > 0 && opponentSizes[nextIdx] <= 2)
    {
        for (int i = 0; i < self->get_size(); i++)
        {
            card c = self->peek(i);
            if (!::canPlayCard(c, current)) continue;
            if (c.number == 11 || c.number == 12 || ::isStackCard(c))
                return i;
        }
    }
    return -1;
}

int HardBotStrategy::findPointMinimization(player * self, const card & current) const
{
    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (!::canPlayCard(c, current)) continue;
        if (::isSpecialCard(c))
            return i;
    }

    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (!::canPlayCard(c, current)) continue;
        if (::isActionCard(c))
            return i;
    }

    return -1;
}

int HardBotStrategy::pickCard(player * self, const card & current,
                              int direction, int selfIdx,
                              const int * opponentSizes, int opponentCount,
                              int drawStack)
{
    if (self->get_size() == 0) return -1;

    detectDrewCards(opponentSizes, opponentCount, current);

    int stackDef = findStackDefense(self, current, drawStack);
    if (stackDef >= 0) return stackDef;

    int threatBlock = findThreatBlock(self, current, direction, selfIdx,
                                      opponentSizes, opponentCount);
    if (threatBlock >= 0) return threatBlock;

    int numberCard = -1;
    int actionCard = -1;
    int wildCard = -1;

    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (!::canPlayCard(c, current)) continue;

        if (c.number == 13 || c.number == 14)
            wildCard = i;
        else if (::isActionCard(c))
            actionCard = i;
        else
            numberCard = i;
    }

    if (numberCard >= 0) return numberCard;
    if (actionCard >= 0) return actionCard;
    if (wildCard >= 0)   return wildCard;

    return -1;
}

COLOR HardBotStrategy::pickColor(player * self)
{
    int bestCount = -1;
    int bestCol = 1;

    for (int col = 1; col <= 4; col++)
    {
        int total = forcedColors[col - 1];

        for (int i = 0; i < self->get_size(); i++)
        {
            card c = self->peek(i);
            if (c.color == col && c.number < 13)
                total += 3;
            else if (c.color == col)
                total += 1;
        }

        if (total > bestCount)
        {
            bestCount = total;
            bestCol = col;
        }
    }

    return static_cast<COLOR>(bestCol);
}

bool HardBotStrategy::shouldJumpIn(player * self, const card & target)
{
    for (int i = 0; i < self->get_size(); i++)
    {
        card c = self->peek(i);
        if (::canJumpIn(c, target))
            return true;
    }
    return false;
}

std::unique_ptr<IBotStrategy> HardBotStrategy::clone() const
{
    return std::make_unique<HardBotStrategy>(*this);
}
