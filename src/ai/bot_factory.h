#ifndef BOT_FACTORY_H
#define BOT_FACTORY_H

#include "ibot_strategy.h"
#include "player.h"
#include <memory>

std::unique_ptr<IBotStrategy> createBotStrategy(BotDifficulty diff);

#endif
