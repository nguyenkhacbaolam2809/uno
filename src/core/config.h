#ifndef CONFIG_H
#define CONFIG_H

enum OSType { OS_UBUNTU, OS_WINDOWS };
enum Language { LANG_ENGLISH, LANG_VIETNAMESE };
struct GameConfig {
    OSType os;
    Language lang;
};

#endif
