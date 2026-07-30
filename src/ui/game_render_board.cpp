#include "game_view.h"
#include "card_renderer.h"
#include "colors.h"
#include <cstring>

using uno::SCREEN_W;
using uno::SCREEN_H;
using uno::CARD_WIDTH;
using uno::CARD_HEIGHT;
using uno::BG_GREEN;
using uno::GOLD_COLOR;

void GameView::renderBackground()
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, BG_GREEN);

    for (int i = 0; i < 20; i++)
    {
        int x = (i * 87 + 43) % SCREEN_W;
        int y = (i * 131 + 17) % SCREEN_H;
        DrawCircle(x, y, 1.5f, Fade(WHITE, 0.08f));
    }
}

void GameView::renderPiles(const GameEngine & engine)
{
    int cx = SCREEN_W / 2, cy = SCREEN_H / 2 - 20;

    int pileW = static_cast<int>(CARD_WIDTH * 1.1f);
    int pileH = static_cast<int>(CARD_HEIGHT * 1.1f);

    int drawX = cx - pileW - 60;
    int drawY = cy - pileH / 2;

    Color pileBg = { 0, 0, 0, 100 };
    Rectangle pileRect = { (float)drawX, (float)drawY, (float)pileW, (float)pileH };
    DrawRectangleRounded(pileRect, 0.3f, 10, pileBg);
    card_renderer::drawBack(drawX + 4, drawY + 4, 1.0f);
    DrawText("DRAW", drawX + 10, drawY + pileH + 8, 14, Fade(WHITE, 0.6f));

    int discX = cx + 20;
    int discY = cy - pileH / 2;

    Color discBg = { 0, 0, 0, 80 };
    Rectangle discRect = { (float)discX, (float)discY, (float)pileW, (float)pileH };
    DrawRectangleRounded(discRect, 0.3f, 10, discBg);

    Card current = engine.getCurrentCard();
    card_renderer::drawCard(current, discX + 4, discY + 4, 1.0f);
    DrawText("PLAY", discX + 10, discY + pileH + 8, 14, Fade(WHITE, 0.6f));

    if (engine.isForceDraw())
    {
        std::string forced = TextFormat("FORCED DRAW: %d", engine.getDrawStack());
        int fw = MeasureText(forced.c_str(), 20);
        Color forcedColor = { 255, 100, 100, 255 };
        DrawText(forced.c_str(), cx - fw / 2, cy + pileH / 2 + 30, 20, forcedColor);
    }

    int dirX = cx - 10;
    int dirY = cy - pileH / 2 - 40;
    const char * dirText = engine.getDirection() == 1 ? "\xe2\x86\x91" : "\xe2\x86\x93";
    DrawText(dirText, dirX, dirY, 32, Fade(GOLD_COLOR, 0.8f));
}
