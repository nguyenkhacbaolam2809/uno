#ifndef RULES_H
#define RULES_H

#include "card.h"

int getStackValue(const card & c);
bool isStackCard(const card & c);
bool isActionCard(const card & c);
bool isSpecialCard(const card & c);
bool canPlayCard(const card & played, const card & current);
bool canJumpIn(const card & played, const card & current);
bool isLegalLastCard(const card & c);

#endif
