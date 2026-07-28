#include "card_renderer.h"
#include "rules.h"
#include <cstdio>
#include <algorithm>

using uno::CARD_WIDTH;
using uno::CARD_HEIGHT;
using uno::CARD_RADIUS;
using uno::WILD_BG;

namespace card_renderer {

static const char * cardLabel(const card & c)
{
    switch (c.number)
    {
        case CARD_DRAW_TWO:       return "+2";
        case CARD_SKIP:           return "\xe2\x9c\x97";  // ✗
        case CARD_REVERSE:        return "\xe2\x86\xbb";  // ↻
        case CARD_WILD:           return "W";
        case CARD_WILD_DRAW_FOUR: return "+4";
        default:
            return TextFormat("%d", c.number);
    }
}

static void drawCardBody(int x, int y, int w, int h, COLOR col, float scale)
{
    Color color = uno::toRaylibColor(col);
    if (col == wild)
        color = WILD_BG;

    drawRoundedRect(x, y, w, h, static_cast<int>(CARD_RADIUS * scale), color);

    int inset = static_cast<int>(4 * scale);
    Color innerGray = { 80, 80, 80, 255 };
    Color inner = (col == wild) ? innerGray : Fade(color, 0.4f);
    drawRoundedRect(x + inset, y + inset, w - 2 * inset, h - 2 * inset,
                    static_cast<int>(CARD_RADIUS * scale / 2), inner);

    if (col == wild)
    {
        int stripH = static_cast<int>(20 * scale);
        int stripY = y + h / 2 - stripH / 2;
        drawRoundedRect(x + inset, stripY, w - 2 * inset, stripH, 0, RED);
        drawRoundedRect(x + inset, stripY + stripH / 4, w - 2 * inset, stripH / 2, 0, BLUE);
        drawRoundedRect(x + inset, stripY - stripH / 4, w - 2 * inset, stripH / 2, 0, GREEN);
    }
}

static void drawCardText(int x, int y, int w, int h, const card & c, float scale)
{
    const char * label = cardLabel(c);
    int fontSize = static_cast<int>(36 * scale);
    if (c.number >= CARD_DRAW_TWO)
        fontSize = static_cast<int>(28 * scale);

    int textW = MeasureText(label, fontSize);
    int textX = x + (w - textW) / 2;
    int textY = y + (h - fontSize) / 2;

    Color darkYellow = { 60, 40, 0, 255 };
    Color shadowColor = { 0, 0, 0, 100 };
    Color textColor = (c.color == yellow) ? darkYellow : WHITE;

    DrawText(label, textX + 1, textY + 1, fontSize, shadowColor);
    DrawText(label, textX, textY, fontSize, textColor);

    if (c.number == CARD_SKIP || c.number == CARD_REVERSE ||
        c.number == CARD_DRAW_TWO || c.number >= CARD_WILD)
    {
        int smallSz = static_cast<int>(14 * scale);
        const char * sub = "";
        if (c.number == CARD_SKIP) sub = "SKIP";
        else if (c.number == CARD_REVERSE) sub = "REV";
        else if (c.number == CARD_DRAW_TWO) sub = "DRAW";
        else if (c.number == CARD_WILD) sub = "WILD";
        else if (c.number == CARD_WILD_DRAW_FOUR) sub = "WILD";

        int subW = MeasureText(sub, smallSz);
        int subX = x + (w - subW) / 2;
        DrawText(sub, subX, y + h - static_cast<int>(24 * scale), smallSz, Fade(textColor, 0.7f));
    }

    char corner[4];
    std::snprintf(corner, sizeof(corner), "%d", c.number);
    if (c.number > CARD_REVERSE) corner[0] = 0;
    int cornerSz = static_cast<int>(14 * scale);
    if (corner[0])
    {
        DrawText(corner, x + static_cast<int>(6 * scale), y + static_cast<int>(4 * scale),
                 cornerSz, Fade(textColor, 0.8f));
        int cw = MeasureText(corner, cornerSz);
        DrawText(corner, x + w - cw - static_cast<int>(6 * scale),
                 y + h - cornerSz - static_cast<int>(4 * scale),
                 cornerSz, Fade(textColor, 0.8f));
    }
}

void drawCard(const card & c, int x, int y, float scale)
{
    int w = static_cast<int>(CARD_WIDTH * scale);
    int h = static_cast<int>(CARD_HEIGHT * scale);

    Color shadow = { 0, 0, 0, 60 };
    DrawRectangle(x + 1, y + 1, w, h, shadow);
    drawCardBody(x, y, w, h, c.color, scale);
    drawCardText(x, y, w, h, c, scale);

    int border = static_cast<int>(2 * scale);
    Color borderCol = Fade(WHITE, 0.3f);
    Rectangle borderRect = { (float)x, (float)y, (float)w, (float)h };
    DrawRectangleLinesEx(borderRect, (float)border, borderCol);
}

void drawBack(int x, int y, float scale)
{
    int w = static_cast<int>(CARD_WIDTH * scale);
    int h = static_cast<int>(CARD_HEIGHT * scale);

    Color shadow = { 0, 0, 0, 60 };
    Color backColor = { 20, 40, 120, 255 };
    Color borderColor = { 60, 100, 200, 200 };
    Color logoColor = { 255, 255, 255, 180 };
    DrawRectangle(x + 1, y + 1, w, h, shadow);
    drawRoundedRect(x, y, w, h, static_cast<int>(CARD_RADIUS * scale), backColor);
    Rectangle borderRect = { (float)x, (float)y, (float)w, (float)h };
    DrawRectangleLinesEx(borderRect, 2.0f, borderColor);

    int cx = x + w / 2, cy = y + h / 2;
    const char * logo = "UNO";
    int sz = static_cast<int>(24 * scale);
    int lw = MeasureText(logo, sz);
    DrawText(logo, cx - lw / 2, cy - sz / 2, sz, logoColor);
}

void drawRoundedRect(int x, int y, int w, int h, int radius, Color color)
{
    if (radius <= 0)
    {
        DrawRectangle(x, y, w, h, color);
        return;
    }
    Rectangle rect = { (float)x, (float)y, (float)w, (float)h };
    DrawRectangleRounded(rect, (float)radius / (float)std::min(w, h), 20, color);
}

bool isHovered(int x, int y, int w, int h)
{
    Vector2 m = GetMousePosition();
    return m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h;
}

Vector2 getCardPos(int index, int total, int screenW, int baseY, float scale, float overlap)
{
    int cw = static_cast<int>(CARD_WIDTH * scale);
    int totalW = total * static_cast<int>(cw * overlap) + static_cast<int>(cw * (1.0f - overlap));
    int startX = (screenW - totalW) / 2;
    return { (float)(startX + index * (int)(cw * overlap)), (float)baseY };
}

}
