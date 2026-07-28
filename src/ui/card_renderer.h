#ifndef CARD_RENDERER_H
#define CARD_RENDERER_H

#include "raylib.h"
#include "card.h"
#include "colors.h"
#include <string>

namespace card_renderer {

void drawCard(const card & c, int x, int y, float scale = 1.0f);
void drawBack(int x, int y, float scale = 1.0f);
void drawRoundedRect(int x, int y, int w, int h, int radius, Color color);
bool isHovered(int x, int y, int w, int h);
Vector2 getCardPos(int index, int total, int screenW, int baseY, float scale = 1.0f, float overlap = 0.6f);

}

#endif
