#ifndef RULES_H
#define RULES_H

#include "card.h"
#include "player.h"

int getStackValue(const Card & c);
bool isStackCard(const Card & c);
bool isActionCard(const Card & c);
bool isSpecialCard(const Card & c);
bool canPlayCard(const Card & played, const Card & current);
bool canJumpIn(const Card & played, const Card & current);
bool isLegalLastCard(const Card & c);
bool canPlayWildDrawFour(const Card & played, const Card & current, const Player & self);

#endif
