#include "bot_factory.h"
#include "bot_easy.h"
#include "bot_medium.h"
#include "bot_hard.h"

std::unique_ptr<IBotStrategy> createBotStrategy(BotDifficulty diff) noexcept
{
    switch (diff)
    {
        case BotDifficulty::Easy:   return std::make_unique<EasyBotStrategy>();
        case BotDifficulty::Normal: return std::make_unique<MediumBotStrategy>();
        case BotDifficulty::Hard:   return std::make_unique<HardBotStrategy>();
        default:                    return std::make_unique<EasyBotStrategy>();
    }
}
