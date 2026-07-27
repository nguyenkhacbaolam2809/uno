#ifndef BOT_FACTORY_H
#define BOT_FACTORY_H

#include "ibot_strategy.h"
#include "player.h"

IBotStrategy * createBotStrategy(BotDifficulty diff);

#endif
