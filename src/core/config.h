#ifndef CONFIG_H
#define CONFIG_H

#include <string>

enum OSType { OS_UBUNTU, OS_WINDOWS };
enum Language { LANG_ENGLISH, LANG_VIETNAMESE };
struct GameConfig {
    OSType os;
    Language lang;
};

GameConfig promptConfig();
void clearScreen(const GameConfig& config);
std::string msg(const GameConfig& config, int msgId);

#endif
