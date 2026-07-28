#include "bot_factory.h"
#include "bot_easy.h"
#include "bot_medium.h"
#include "bot_hard.h"

std::unique_ptr<IBotStrategy> createBotStrategy(BotDifficulty diff) noexcept
{
    switch (diff)
    {
        case D_EASY:   return std::make_unique<EasyBotStrategy>();
        case D_NORMAL: return std::make_unique<MediumBotStrategy>();
        case D_HARD:   return std::make_unique<HardBotStrategy>();
        default:       return std::make_unique<EasyBotStrategy>();
    }
}
