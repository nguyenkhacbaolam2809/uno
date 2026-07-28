#include "game_view.h"
#include "card_renderer.h"
#include "colors.h"
#include "rules.h"
#include <algorithm>
#include <cstring>

static void drawCentered(const char * text, int y, int fontSize, Color col)
{
    int tw = MeasureText(text, fontSize);
    DrawText(text, (SCREEN_W - tw) / 2, y, fontSize, col);
}

void GameView::renderOpponents(const GameEngine & engine, int localPlayerId)
{
    int n = engine.getPlayerCount();
    int others = n - 1;
    if (others <= 0) return;

    int slotW = 180, slotH = 80;
    int gap = 20;
    int totalW = others * slotW + (others - 1) * gap;
    int startX = (SCREEN_W - totalW) / 2;

    int oppIdx = 0;
    for (int i = 0; i < n; i++)
    {
        if (i == localPlayerId) continue;

        int x = startX + oppIdx * (slotW + gap);
        int y = 20;

        DrawRectangleRounded((Rectangle){ (float)x, (float)y, (float)slotW, (float)slotH },
                             0.3f, 10, Fade(BLACK, 0.3f));
        DrawRectangleRoundedLines((Rectangle){ (float)x, (float)y, (float)slotW, (float)slotH },
                                  0.3f, 10, 1, Fade(WHITE, 0.15f));

        const player * p = engine.getPlayer(i);
        int cardCount = p->get_size();
        bool isCurrentTurn = (engine.getCurrentTurn() % n) == i;
        Color nameCol = isCurrentTurn ? GOLD : WHITE_SMOKE;

        DrawText(p->getName().c_str(), x + 10, y + 8, 18, nameCol);
        DrawText(TextFormat("Cards: %d", cardCount), x + 10, y + 36, 16, Fade(WHITE, 0.7f));

        for (int j = 0; j < std::min(cardCount, 5); j++)
        {
            int bx = x + 10 + j * 32;
            card_renderer::drawBack(bx, y + 56, 0.3f);
        }

        if (p->get_size() == 1)
            DrawText("UNO!", x + slotW - 60, y + 8, 16, (Color){ 255, 200, 0, 255 });

        oppIdx++;
    }

    if (engine.getPhase() == PHASE_GAME_OVER)
    {
        int winner = engine.getWinner();
        if (winner >= 0)
        {
            const player * wp = engine.getPlayer(winner);
            std::string wtxt = wp->getName() + " wins!";
            int tw = MeasureText(wtxt.c_str(), 36);
            DrawRectangle((SCREEN_W - tw) / 2 - 20, SCREEN_H / 2 - 40, tw + 40, 80,
                          (Color){ 0, 0, 0, 180 });
            drawCentered(wtxt.c_str(), SCREEN_H / 2 - 12, 36, GOLD);
        }
    }
}

void GameView::renderTurnIndicator(const GameEngine & engine, int localPlayerId)
{
    int n = engine.getPlayerCount();
    int current = engine.getCurrentTurn() % n;

    if (current == localPlayerId)
    {
        Rectangle btn = { (float)(SCREEN_W / 2 - 50), (float)(SCREEN_H - 75), 100, 30 };
        if (CheckCollisionPointRec(GetMousePosition(), btn))
            DrawRectangleRounded(btn, 0.3f, 10, (Color){ 200, 180, 0, 255 });
        else
            DrawRectangleRounded(btn, 0.3f, 10, (Color){ 180, 140, 20, 255 });
        drawCentered("DRAW", SCREEN_H - 72, 18, WHITE);

        if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            pendingResult.action = PlayerAction::DRAW_CARD;
    }
}
