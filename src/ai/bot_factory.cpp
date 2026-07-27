#include "bot_factory.h"
#include "bot_easy.h"
#include "bot_medium.h"
#include "bot_hard.h"

IBotStrategy * createBotStrategy(BotDifficulty diff)
{
    switch (diff)
    {
        case D_EASY:   return new EasyBotStrategy();
        case D_NORMAL: return new MediumBotStrategy();
        case D_HARD:   return new HardBotStrategy();
        default:       return new EasyBotStrategy();
    }
}
