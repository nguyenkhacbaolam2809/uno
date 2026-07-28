#ifndef UNO_COLORS_H
#define UNO_COLORS_H

#include "raylib.h"

namespace uno {

inline Color toRaylibColor(int c)
{
    switch (c)
    {
        case 1: return (Color){ 237,  28,  36, 255 };
        case 2: return (Color){   0, 155,  72, 255 };
        case 3: return (Color){   0, 114, 188, 255 };
        case 4: return (Color){ 255, 205,   0, 255 };
        default: return (Color){  30,  30,  30, 255 };
    }
}

inline constexpr int CARD_WIDTH  = 80;
inline constexpr int CARD_HEIGHT = 120;
inline constexpr int CARD_RADIUS = 10;

inline constexpr int SCREEN_W = 1280;
inline constexpr int SCREEN_H = 720;

inline constexpr Color BG_GREEN    = {  30,  80,  40, 255 };
inline constexpr Color BG_DARK     = {  20,  20,  30, 255 };
inline constexpr Color WHITE_SMOKE = { 245, 245, 240, 255 };
inline constexpr Color GOLD        = { 255, 215,   0, 255 };
inline constexpr Color WILD_BG     = {  50,  50,  50, 255 };
}

#endif
