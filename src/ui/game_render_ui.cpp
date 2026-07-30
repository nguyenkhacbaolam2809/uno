#include "game_view.h"
#include "card_renderer.h"
#include "rules.h"
#include "colors.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

using uno::SCREEN_W;
using uno::SCREEN_H;
using uno::CARD_WIDTH;
using uno::CARD_HEIGHT;
using uno::BG_GREEN;
using uno::GOLD_COLOR;

void GameView::renderUnoButton(const GameEngine & engine, int localPlayerId)
{
    int n = engine.getPlayerCount();
    int current = engine.getCurrentTurn() % n;
    const Player * p = engine.getPlayer(localPlayerId);
    int handSize = p->get_size();

    unoButtonEnabled = (current == localPlayerId && handSize == 2);
    if (!unoButtonEnabled) return;

    Rectangle btn = { (float)(SCREEN_W / 2 + 60), (float)(SCREEN_H - 80), 100, 40 };
    Color baseColor = { 237, 28, 36, 255 };
    if (CheckCollisionPointRec(GetMousePosition(), btn))
        baseColor = Fade(baseColor, 0.7f);
    DrawRectangleRounded(btn, 0.3f, 10, baseColor);
    DrawRectangleRoundedLines(btn, 0.3f, 10, 2, GOLD_COLOR);
    int utw = MeasureText("UNO!", 22);
    DrawText("UNO!", (SCREEN_W - utw) / 2, SCREEN_H - 74, 22, GOLD_COLOR);

    if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        pendingResult.action = PlayerAction::SAY_UNO;
        pendingResult.targetId = localPlayerId;
    }
}

void GameView::renderCatchTargets(const GameEngine & engine, int localPlayerId)
{
    int n = engine.getPlayerCount();
    vulnerableOpponent = -1;

    for (int i = 0; i < n; i++)
    {
        if (i == localPlayerId) continue;
        const Player * p = engine.getPlayer(i);
        if (p->get_size() == 1)
        {
            vulnerableOpponent = i;
            break;
        }
    }

    if (vulnerableOpponent < 0) return;

    int slotW = 180;
    int gap = 20;
    int others = n - 1;
    int totalW = others * slotW + (others - 1) * gap;
    if (totalW < 0) totalW = 0;
    int startX = (SCREEN_W - totalW) / 2;

    int oppIdx = 0;
    for (int i = 0; i < n; i++)
    {
        if (i == localPlayerId) continue;
        if (i == vulnerableOpponent)
        {
            int x = startX + oppIdx * (slotW + gap);
            Rectangle r = { (float)x, (float)110, (float)slotW, 30 };
            Color catchBg = { 237, 28, 36, 200 };
            DrawRectangleRounded(r, 0.3f, 10, catchBg);
            int tw = MeasureText("CATCH UNO?", 16);
            DrawText("CATCH UNO?", x + (slotW - tw) / 2, 114, 16, GOLD_COLOR);
        }
        oppIdx++;
    }
}

void GameView::renderColorPicker()
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.7f));

    const char * title = "Choose a color";
    int tw = MeasureText(title, 28);
    DrawText(title, (SCREEN_W - tw) / 2, SCREEN_H / 2 - 120, 28, WHITE);

    Color redCol = { 237, 28, 36, 255 };
    Color blueCol = { 0, 114, 188, 255 };
    Color greenCol = { 0, 155, 72, 255 };
    Color yellowCol = { 255, 205, 0, 255 };
    struct { CardColor col; Color c; const char * label; } colors[4] = {
        { CardColor::Red,    redCol,    "RED" },
        { CardColor::Blue,   blueCol,   "BLUE" },
        { CardColor::Green,  greenCol,  "GREEN" },
        { CardColor::Yellow, yellowCol, "YELLOW" }
    };

    int btnW = 120, btnH = 80, gap = 20;
    int totalW = 4 * btnW + 3 * gap;
    int startX = (SCREEN_W - totalW) / 2;
    int startY = SCREEN_H / 2 - btnH / 2;

    for (int i = 0; i < 4; i++)
    {
        Rectangle r = { (float)(startX + i * (btnW + gap)), (float)startY, (float)btnW, (float)btnH };
        Color c = colors[i].c;
        if (CheckCollisionPointRec(GetMousePosition(), r))
            c = Fade(c, 0.7f);
        DrawRectangleRounded(r, 0.3f, 10, c);

        int lw = MeasureText(colors[i].label, 16);
        DrawText(colors[i].label, (int)(r.x + (r.width - lw) / 2), (int)(r.y + (r.height - 20) / 2), 16, WHITE);

        if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            pickedColor = colors[i].col;
            pendingResult.action = PlayerAction::PLAY_CARD;
            pendingResult.cardIndex = selectedCard;
            pendingResult.chosenColor = pickedColor;
            needsColorPick = false;
        }
    }
}

void GameView::renderMessageOverlay()
{
    if (overlayTimer <= 0) return;
    overlayTimer -= GetFrameTime();

    int tw = MeasureText(overlayMsg.c_str(), 24);
    int mx = (SCREEN_W - tw) / 2 - 20;
    int my = SCREEN_H / 2 - 30;
    Color overlayBg = { 0, 0, 0, 200 };
    DrawRectangle(mx, my, tw + 40, 60, overlayBg);
    DrawText(overlayMsg.c_str(), mx + 20, my + 18, 24, WHITE);
}

void GameView::handleHandClick(const GameEngine & engine, int localPlayerId)
{
    if (pendingResult.action != PlayerAction::NONE) return;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    int n = engine.getPlayerCount();
    bool isMyTurn = (engine.getCurrentTurn() % n) == localPlayerId;
    if (!isMyTurn) return;

    const Player * p = engine.getPlayer(localPlayerId);
    int handSize = p->get_size();

    for (int i = 0; i < handSize; i++)
    {
        Vector2 pos = getHandCardPos(i, handSize);
        Rectangle r = { pos.x, pos.y, (float)CARD_WIDTH, (float)CARD_HEIGHT };

        if (CheckCollisionPointRec(GetMousePosition(), r))
        {
            Card c = p->peek(i);
            Card current = engine.getCurrentCard();
            if (canPlayCard(c, current))
            {
                if (c.color == CardColor::Wild)
                {
                    needsColorPick = true;
                    selectedCard = i;
                }
                else
                {
                    pendingResult.action = PlayerAction::PLAY_CARD;
                    pendingResult.cardIndex = i;
                    pendingResult.chosenColor = c.color;
                }
            }
            return;
        }
    }
}

void GameView::handleUnoCatchClick(const GameEngine & engine, int localPlayerId)
{
    if (pendingResult.action != PlayerAction::NONE) return;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    if (vulnerableOpponent >= 0)
    {
        int n = engine.getPlayerCount();
        int slotW = 180, slotH = 80;
        int gap = 20;
        int others = n - 1;
        if (others > 0)
        {
            int totalW = others * slotW + (others - 1) * gap;
            int startX = (SCREEN_W - totalW) / 2;
            int oppIdx = 0;
            for (int i = 0; i < n; i++)
            {
                if (i == localPlayerId) continue;
                if (i == vulnerableOpponent)
                {
                    int x = startX + oppIdx * (slotW + gap);
                    Rectangle r = { (float)x, 20, (float)slotW, (float)slotH };
                    if (CheckCollisionPointRec(GetMousePosition(), r))
                    {
                        pendingResult.action = PlayerAction::CATCH_UNO;
                        pendingResult.targetId = i;
                        return;
                    }
                }
                oppIdx++;
            }
        }
    }
}


