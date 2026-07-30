#ifndef UNO_COLORS_H
#define UNO_COLORS_H

#include "raylib.h"
#include "card.h"

namespace uno {

inline const Color COLOR_RED    = { 237,  28,  36, 255 };
inline const Color COLOR_GREEN  = {   0, 155,  72, 255 };
inline const Color COLOR_BLUE   = {   0, 114, 188, 255 };
inline const Color COLOR_YELLOW = { 255, 205,   0, 255 };
inline const Color COLOR_DARK   = {  30,  30,  30, 255 };

inline Color toRaylibColor(CardColor c)
{
    switch (c)
    {
        case CardColor::Red:    return COLOR_RED;
        case CardColor::Green:  return COLOR_GREEN;
        case CardColor::Blue:   return COLOR_BLUE;
        case CardColor::Yellow: return COLOR_YELLOW;
        default:                return COLOR_DARK;
    }
}

inline constexpr int CARD_WIDTH  = 80;
inline constexpr int CARD_HEIGHT = 120;
inline constexpr int CARD_RADIUS = 10;

inline constexpr int SCREEN_W = 1280;
inline constexpr int SCREEN_H = 720;

inline const Color BG_GREEN    = {  30,  80,  40, 255 };
inline const Color BG_DARK     = {  20,  20,  30, 255 };
inline const Color WHITE_SMOKE = { 245, 245, 240, 255 };
inline const Color GOLD_COLOR  = { 255, 215,   0, 255 };
inline const Color WILD_BG     = {  50,  50,  50, 255 };
}

#endif
