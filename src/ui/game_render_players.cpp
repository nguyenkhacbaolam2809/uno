#include "game_view.h"
#include "card_renderer.h"
#include "colors.h"
#include "rules.h"
#include <algorithm>
#include <cstring>

using uno::SCREEN_W;
using uno::SCREEN_H;
using uno::WHITE_SMOKE;
using uno::GOLD_COLOR;

void drawOpponentSlot(int x, int y, int slotW, int slotH,
                      const std::string & name, int cardCount,
                      bool isCurrentTurn, bool hasOneCard)
{
    Rectangle slotRect = { (float)x, (float)y, (float)slotW, (float)slotH };
    DrawRectangleRounded(slotRect, 0.3f, 10, Fade(BLACK, 0.3f));
    DrawRectangleRoundedLines(slotRect, 0.3f, 10, 1, Fade(WHITE, 0.15f));

    Color nameCol = isCurrentTurn ? GOLD_COLOR : WHITE_SMOKE;
    DrawText(name.c_str(), x + 10, y + 8, 18, nameCol);
    DrawText(TextFormat("Cards: %d", cardCount), x + 10, y + 36, 16, Fade(WHITE, 0.7f));

    for (int j = 0; j < std::min(cardCount, 5); j++)
        card_renderer::drawBack(x + 10 + j * 32, y + 56, 0.3f);

    if (hasOneCard)
    {
        Color unoColor = { 255, 200, 0, 255 };
        DrawText("UNO!", x + slotW - 60, y + 8, 16, unoColor);
    }
}

void drawGameOverText(const std::string & winnerText)
{
    int tw = MeasureText(winnerText.c_str(), 36);
    Color overlayBg = { 0, 0, 0, 180 };
    DrawRectangle((SCREEN_W - tw) / 2 - 20, SCREEN_H / 2 - 40, tw + 40, 80, overlayBg);
    DrawText(winnerText.c_str(), (SCREEN_W - tw) / 2, SCREEN_H / 2 - 12, 36, GOLD_COLOR);
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
        const Player * p = engine.getPlayer(i);
        int cardCount = p->get_size();
        bool isCurrent = (engine.getCurrentTurn() % n) == i;
        drawOpponentSlot(x, 20, slotW, slotH, p->getName(), cardCount, isCurrent, p->get_size() == 1);
        oppIdx++;
    }

    if (engine.getPhase() == GamePhase::GameOver)
    {
        int winner = engine.getWinner();
        if (winner >= 0)
        {
            const Player * wp = engine.getPlayer(winner);
            drawGameOverText(wp->getName() + " wins!");
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
        Color drawHover = { 200, 180, 0, 255 };
        Color drawNormal = { 180, 140, 20, 255 };
        DrawRectangleRounded(btn, 0.3f, 10, CheckCollisionPointRec(GetMousePosition(), btn) ? drawHover : drawNormal);
        int dw = MeasureText("DRAW", 18);
        DrawText("DRAW", (SCREEN_W - dw) / 2, SCREEN_H - 72, 18, WHITE);

        if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            pendingResult.action = PlayerAction::DRAW_CARD;
    }
}
