#include "game_view.h"
#include "card_renderer.h"
#include "rules.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

static void drawTextCentered(const char * text, int y, int fontSize, Color col)
{
    int tw = MeasureText(text, fontSize);
    DrawText(text, (SCREEN_W - tw) / 2, y, fontSize, col);
}

GameView::GameView()
    : hoveredCard(-1), selectedCard(-1), needsColorPick(false),
      pickedColor(wild), showUnoButton(false), catchTarget(-1), overlayTimer(0.0f)
{
}

void GameView::resetInteraction()
{
    pendingResult.action = PlayerAction::NONE;
    pendingResult.cardIndex = -1;
    pendingResult.targetId = -1;
    pendingResult.chosenColor = wild;
    selectedCard = -1;
    hoveredCard = -1;
    needsColorPick = false;
}

InteractionResult GameView::getInteraction()
{
    InteractionResult r = pendingResult;
    if (r.action != PlayerAction::NONE)
        resetInteraction();
    return r;
}

void GameView::showMessage(const std::string & msg, float duration)
{
    overlayMsg = msg;
    overlayTimer = duration;
}

bool GameView::isReady() const
{
    return true;
}

void GameView::render(const GameEngine & engine, int localPlayerId)
{
    renderBackground();
    renderOpponents(engine, localPlayerId);
    renderPiles(engine);
    renderHand(engine, localPlayerId);
    renderTurnIndicator(engine, localPlayerId);

    if (needsColorPick)
        renderColorPicker();

    if (overlayTimer > 0)
        renderMessageOverlay();

    handleHandClick(engine, localPlayerId);
}

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
        {
            DrawText("UNO!", x + slotW - 60, y + 8, 16, (Color){ 255, 200, 0, 255 });
        }

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
            drawTextCentered(wtxt.c_str(), SCREEN_H / 2 - 12, 36, GOLD);
        }
    }
}

void GameView::renderPiles(const GameEngine & engine)
{
    int cx = SCREEN_W / 2, cy = SCREEN_H / 2 - 20;

    int pileW = static_cast<int>(CARD_WIDTH * 1.1f);
    int pileH = static_cast<int>(CARD_HEIGHT * 1.1f);

    // Draw pile (left)
    int drawX = cx - pileW - 60;
    int drawY = cy - pileH / 2;

    DrawRectangleRounded((Rectangle){ (float)drawX, (float)drawY, (float)pileW, (float)pileH },
                         0.3f, 10, (Color){ 0, 0, 0, 100 });
    card_renderer::drawBack(drawX + 4, drawY + 4, 1.0f);

    DrawText("DRAW", drawX + 10, drawY + pileH + 8, 14, Fade(WHITE, 0.6f));

    // Discard pile (center)
    int discX = cx + 20;
    int discY = cy - pileH / 2;

    DrawRectangleRounded((Rectangle){ (float)discX, (float)discY, (float)pileW, (float)pileH },
                         0.3f, 10, (Color){ 0, 0, 0, 80 });

    card current = engine.getCurrentCard();
    card_renderer::drawCard(current, discX + 4, discY + 4, 1.0f);

    DrawText("PLAY", discX + 10, discY + pileH + 8, 14, Fade(WHITE, 0.6f));

    // Force draw indicator
    if (engine.isForceDraw())
    {
        std::string forced = TextFormat("FORCED DRAW: %d", engine.getDrawStack());
        int fw = MeasureText(forced.c_str(), 20);
        DrawText(forced.c_str(), cx - fw / 2, cy + pileH / 2 + 30, 20, (Color){ 255, 100, 100, 255 });
    }

    // Direction indicator
    const char * dirText = engine.getDirection() == 1 ? "\xe2\x86\x91" : "\xe2\x86\x93";
    int dirX = cx - 10;
    int dirY = cy - pileH / 2 - 40;
    DrawText(dirText, dirX, dirY, 32, Fade(GOLD, 0.8f));
}

void GameView::renderHand(const GameEngine & engine, int localPlayerId)
{
    const player * p = engine.getPlayer(localPlayerId);
    int handSize = p->get_size();
    if (handSize == 0) return;

    bool isMyTurn = (engine.getCurrentTurn() % engine.getPlayerCount()) == localPlayerId;

    Vector2 m = GetMousePosition();
    hoveredCard = -1;

    for (int i = 0; i < handSize; i++)
    {
        Vector2 pos = getHandCardPos(i, handSize);
        Rectangle r = { pos.x, pos.y, (float)CARD_WIDTH, (float)CARD_HEIGHT };

        bool hov = CheckCollisionPointRec(m, r);
        float lift = 0.0f;

        if (isMyTurn && hov)
        {
            lift = 20.0f;
            hoveredCard = i;
        }

        if (hov && isMyTurn)
        {
            DrawRectangleRounded((Rectangle){ pos.x - 2, pos.y - 2, (float)CARD_WIDTH + 4,
                                              (float)CARD_HEIGHT + 4 + lift },
                                 0.3f, 10, Fade(GOLD, 0.4f));
        }

        card c = p->peek(i);
        card_renderer::drawCard(c, (int)pos.x, (int)(pos.y - lift), 1.0f);
    }
}

Vector2 GameView::getHandCardPos(int index, int total)
{
    float overlap = (total > 7) ? 50.0f : 70.0f;
    float totalW = overlap * (total - 1) + CARD_WIDTH;
    float startX = (SCREEN_W - totalW) / 2.0f;
    float baseY = (float)(SCREEN_H - CARD_HEIGHT - 30);
    return { startX + index * overlap, baseY };
}

Rectangle GameView::getCardRect(int index, int total)
{
    Vector2 p = getHandCardPos(index, total);
    return { p.x, p.y, (float)CARD_WIDTH, (float)CARD_HEIGHT };
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
        drawTextCentered("DRAW", SCREEN_H - 72, 18, WHITE);

        if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            pendingResult.action = PlayerAction::DRAW_CARD;
        }
    }
}

void GameView::renderColorPicker()
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.7f));

    const char * title = "Choose a color";
    int tw = MeasureText(title, 28);
    DrawText(title, (SCREEN_W - tw) / 2, SCREEN_H / 2 - 120, 28, WHITE);

    struct { COLOR col; Color c; const char * label; } colors[4] = {
        { red,    (Color){ 237, 28, 36, 255 },   "RED" },
        { blue,   (Color){ 0, 114, 188, 255 },   "BLUE" },
        { green,  (Color){ 0, 155, 72, 255 },    "GREEN" },
        { yellow, (Color){ 255, 205, 0, 255 },   "YELLOW" }
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
        DrawText(colors[i].label, (int)(r.x + (r.w - lw) / 2), (int)(r.y + (r.h - 20) / 2), 16, WHITE);

        if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            pickedColor = colors[i].col;
            pendingResult.chosenColor = pickedColor;
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
    DrawRectangle(mx, my, tw + 40, 60, (Color){ 0, 0, 0, 200 });
    DrawText(overlayMsg.c_str(), mx + 20, my + 18, 24, WHITE);
}

void GameView::handleHandClick(const GameEngine & engine, int localPlayerId)
{
    if (pendingResult.action != PlayerAction::NONE) return;

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    int n = engine.getPlayerCount();
    bool isMyTurn = (engine.getCurrentTurn() % n) == localPlayerId;
    if (!isMyTurn) return;

    const player * p = engine.getPlayer(localPlayerId);
    int handSize = p->get_size();

    for (int i = 0; i < handSize; i++)
    {
        Vector2 pos = getHandCardPos(i, handSize);
        Rectangle r = { pos.x, pos.y, (float)CARD_WIDTH, (float)CARD_HEIGHT };

        if (CheckCollisionPointRec(GetMousePosition(), r))
        {
            card c = p->peek(i);
            card current = engine.getCurrentCard();
            if (canPlayCard(c, current))
            {
                if (c.color == wild)
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

void GameView::showGameOver(const GameEngine & engine)
{
    int winner = engine.getWinner();
    if (winner < 0) return;

    const player * wp = engine.getPlayer(winner);
    std::string msg = wp->getName() + " wins!";

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BG_GREEN);

        int tw = MeasureText(msg.c_str(), 48);
        DrawText(msg.c_str(), (SCREEN_W - tw) / 2, SCREEN_H / 2 - 60, 48, GOLD);

        const char * sub = "Press any key to continue";
        int sw = MeasureText(sub, 20);
        DrawText(sub, (SCREEN_W - sw) / 2, SCREEN_H / 2 + 20, 20, Fade(WHITE, 0.7f));

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            break;

        EndDrawing();
    }
}

void GameView::renderSync(const SyncState & state, int localPlayerId)
{
    renderBackground();

    int n = (int)state.players.size();
    int slotW = 180, slotH = 80;
    int gap = 20;
    int totalW = (n - 1) * slotW + (n - 2) * gap;
    if (totalW < 0) totalW = 0;
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

        bool isCurrent = (state.gs.turn % n) == i;
        Color nameCol = isCurrent ? GOLD : WHITE_SMOKE;

        DrawText(state.players[i].name.c_str(), x + 10, y + 8, 18, nameCol);
        int cc = (int)state.players[i].hand.size();
        DrawText(TextFormat("Cards: %d", cc), x + 10, y + 36, 16, Fade(WHITE, 0.7f));

        for (int j = 0; j < std::min(cc, 5); j++)
            card_renderer::drawBack(x + 10 + j * 32, y + 56, 0.3f);

        if (cc == 1)
            DrawText("UNO!", x + slotW - 60, y + 8, 16, (Color){ 255, 200, 0, 255 });
        oppIdx++;
    }

    if (state.gs.phase == PHASE_GAME_OVER)
    {
        int winner = state.gs.winner;
        if (winner >= 0 && winner < n)
        {
            std::string wtxt = state.players[winner].name + " wins!";
            int tw = MeasureText(wtxt.c_str(), 36);
            DrawRectangle((SCREEN_W - tw) / 2 - 20, SCREEN_H / 2 - 40, tw + 40, 80,
                          (Color){ 0, 0, 0, 180 });
            drawTextCentered(wtxt.c_str(), SCREEN_H / 2 - 12, 36, GOLD);
        }
        return;
    }

    int cx = SCREEN_W / 2, cy = SCREEN_H / 2 - 20;
    int pileW = (int)(CARD_WIDTH * 1.1f);
    int pileH = (int)(CARD_HEIGHT * 1.1f);

    int drawX = cx - pileW - 60;
    int drawY = cy - pileH / 2;
    DrawRectangleRounded((Rectangle){ (float)drawX, (float)drawY, (float)pileW, (float)pileH },
                         0.3f, 10, (Color){ 0, 0, 0, 100 });
    card_renderer::drawBack(drawX + 4, drawY + 4, 1.0f);
    DrawText("DRAW", drawX + 10, drawY + pileH + 8, 14, Fade(WHITE, 0.6f));

    int discX = cx + 20;
    int discY = cy - pileH / 2;
    DrawRectangleRounded((Rectangle){ (float)discX, (float)discY, (float)pileW, (float)pileH },
                         0.3f, 10, (Color){ 0, 0, 0, 80 });
    card_renderer::drawCard(state.gs.currentCard, discX + 4, discY + 4, 1.0f);
    DrawText("PLAY", discX + 10, discY + pileH + 8, 14, Fade(WHITE, 0.6f));

    if (state.gs.forceDraw)
    {
        std::string forced = TextFormat("FORCED DRAW: %d", state.gs.drawStack);
        int fw = MeasureText(forced.c_str(), 20);
        DrawText(forced.c_str(), cx - fw / 2, cy + pileH / 2 + 30, 20, (Color){ 255, 100, 100, 255 });
    }

    const char * dirText = state.gs.direction == 1 ? "\xe2\x86\x91" : "\xe2\x86\x93";
    DrawText(dirText, cx - 10, cy - pileH / 2 - 40, 32, Fade(GOLD, 0.8f));

    const SyncPlayer & me = state.players[localPlayerId];
    int handSize = (int)me.hand.size();
    bool isMyTurn = (state.gs.turn % n) == localPlayerId;
    Vector2 m = GetMousePosition();
    hoveredCard = -1;

    for (int i = 0; i < handSize; i++)
    {
        Vector2 pos = getHandCardPos(i, handSize);
        float lift = (isMyTurn && CheckCollisionPointRec(m,
            (Rectangle){ pos.x, pos.y, (float)CARD_WIDTH, (float)CARD_HEIGHT })) ? 20.0f : 0.0f;

        if (lift > 0 && isMyTurn)
        {
            hoveredCard = i;
            DrawRectangleRounded((Rectangle){ pos.x - 2, pos.y - 2 - lift,
                                              (float)CARD_WIDTH + 4, (float)CARD_HEIGHT + 4 + lift },
                                 0.3f, 10, Fade(GOLD, 0.4f));
        }

        card_renderer::drawCard(me.hand[i], (int)pos.x, (int)(pos.y - lift), 1.0f);
    }

    if (isMyTurn)
    {
        Rectangle btn = { (float)(SCREEN_W / 2 - 50), (float)(SCREEN_H - 75), 100, 30 };
        if (CheckCollisionPointRec(m, btn))
            DrawRectangleRounded(btn, 0.3f, 10, (Color){ 200, 180, 0, 255 });
        else
            DrawRectangleRounded(btn, 0.3f, 10, (Color){ 180, 140, 20, 255 });
        drawTextCentered("DRAW", SCREEN_H - 72, 18, WHITE);

        if (CheckCollisionPointRec(m, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            pendingResult.action = PlayerAction::DRAW_CARD;
    }

    if (needsColorPick)
        renderColorPicker();

    if (overlayTimer > 0)
        renderMessageOverlay();

    if (isMyTurn && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        for (int i = 0; i < handSize; i++)
        {
            Vector2 pos = getHandCardPos(i, handSize);
            if (CheckCollisionPointRec(m,
                (Rectangle){ pos.x, pos.y, (float)CARD_WIDTH, (float)CARD_HEIGHT }))
            {
                card chosen = me.hand[i];
                if (canPlayCard(chosen, state.gs.currentCard))
                {
                    if (chosen.color == wild)
                    {
                        needsColorPick = true;
                        selectedCard = i;
                    }
                    else
                    {
                        pendingResult.action = PlayerAction::PLAY_CARD;
                        pendingResult.cardIndex = i;
                        pendingResult.chosenColor = chosen.color;
                    }
                }
                return;
            }
        }
    }
}
