#include "game_view.h"
#include "card_renderer.h"
#include "rules.h"
#include "particle_system.h"
#include "animation_manager.h"
#include "colors.h"
#include <algorithm>

using uno::SCREEN_W;
using uno::SCREEN_H;
using uno::CARD_WIDTH;
using uno::CARD_HEIGHT;
using uno::BG_GREEN;
using uno::GOLD_COLOR;
using uno::WHITE_SMOKE;
using uno::WILD_BG;

GameView::GameView()
    : hoveredCard(-1), selectedCard(-1), needsColorPick(false),
      pickedColor(wild),
      overlayTimer(0.0f), unoButtonEnabled(false), vulnerableOpponent(-1),
      handScrollOffset(0)
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

bool GameView::isReady() const noexcept
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
    renderUnoButton(engine, localPlayerId);
    renderCatchTargets(engine, localPlayerId);

    if (needsColorPick)
        renderColorPicker();

    if (overlayTimer > 0)
        renderMessageOverlay();

    ParticleSystem::instance().update(GetFrameTime());
    ParticleSystem::instance().render();

    AnimationManager::instance().update(GetFrameTime());

    handleHandClick(engine, localPlayerId);
    handleUnoCatchClick(engine, localPlayerId);
}

#ifndef _WIN32
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

        Rectangle slotRect = { (float)x, (float)y, (float)slotW, (float)slotH };
        DrawRectangleRounded(slotRect, 0.3f, 10, Fade(BLACK, 0.3f));
        DrawRectangleRoundedLines(slotRect, 0.3f, 10, 1, Fade(WHITE, 0.15f));

        bool isCurrent = (state.gs.turn % n) == i;
        Color nameCol = isCurrent ? GOLD_COLOR : WHITE_SMOKE;

        DrawText(state.players[i].name.c_str(), x + 10, y + 8, 18, nameCol);
        int cc = (int)state.players[i].hand.size();
        DrawText(TextFormat("Cards: %d", cc), x + 10, y + 36, 16, Fade(WHITE, 0.7f));

        for (int j = 0; j < std::min(cc, 5); j++)
            card_renderer::drawBack(x + 10 + j * 32, y + 56, 0.3f);

        if (cc == 1)
        {
            Color unoTextColor = { 255, 200, 0, 255 };
            DrawText("UNO!", x + slotW - 60, y + 8, 16, unoTextColor);
        }
        oppIdx++;
    }

    if (state.gs.phase == PHASE_GAME_OVER)
    {
        int winner = state.gs.winner;
        if (winner >= 0 && winner < n)
        {
            std::string wtxt = state.players[winner].name + " wins!";
            int tw = MeasureText(wtxt.c_str(), 36);
            Color overlayBg = { 0, 0, 0, 180 };
            DrawRectangle((SCREEN_W - tw) / 2 - 20, SCREEN_H / 2 - 40, tw + 40, 80, overlayBg);
            int tw2 = MeasureText(wtxt.c_str(), 36);
            DrawText(wtxt.c_str(), (SCREEN_W - tw2) / 2, SCREEN_H / 2 - 12, 36, GOLD_COLOR);
        }
        return;
    }

    int cx = SCREEN_W / 2, cy = SCREEN_H / 2 - 20;
    int pileW = (int)(CARD_WIDTH * 1.1f);
    int pileH = (int)(CARD_HEIGHT * 1.1f);

    int drawX = cx - pileW - 60;
    int drawY = cy - pileH / 2;
    Rectangle drawPileRect = { (float)drawX, (float)drawY, (float)pileW, (float)pileH };
    Color drawPileBg = { 0, 0, 0, 100 };
    DrawRectangleRounded(drawPileRect, 0.3f, 10, drawPileBg);
    card_renderer::drawBack(drawX + 4, drawY + 4, 1.0f);
    DrawText("DRAW", drawX + 10, drawY + pileH + 8, 14, Fade(WHITE, 0.6f));

    int discX = cx + 20;
    int discY = cy - pileH / 2;
    Rectangle discPileRect = { (float)discX, (float)discY, (float)pileW, (float)pileH };
    Color discPileBg = { 0, 0, 0, 80 };
    DrawRectangleRounded(discPileRect, 0.3f, 10, discPileBg);
    card_renderer::drawCard(state.gs.currentCard, discX + 4, discY + 4, 1.0f);
    DrawText("PLAY", discX + 10, discY + pileH + 8, 14, Fade(WHITE, 0.6f));

    if (state.gs.forceDraw)
    {
        std::string forced = TextFormat("FORCED DRAW: %d", state.gs.drawStack);
        int fw = MeasureText(forced.c_str(), 20);
        Color forcedColor = { 255, 100, 100, 255 };
        DrawText(forced.c_str(), cx - fw / 2, cy + pileH / 2 + 30, 20, forcedColor);
    }

    const char * dirText = state.gs.direction == 1 ? "\xe2\x86\x91" : "\xe2\x86\x93";
    DrawText(dirText, cx - 10, cy - pileH / 2 - 40, 32, Fade(GOLD_COLOR, 0.8f));

    const SyncPlayer & me = state.players[localPlayerId];
    int handSize = (int)me.hand.size();
    bool isMyTurn = (state.gs.turn % n) == localPlayerId;
    Vector2 m = GetMousePosition();
    hoveredCard = -1;

    for (int i = 0; i < handSize; i++)
    {
        Vector2 pos = getHandCardPos(i, handSize);
        Rectangle cardHit = { pos.x, pos.y, (float)CARD_WIDTH, (float)CARD_HEIGHT };
        float lift = (isMyTurn && CheckCollisionPointRec(m, cardHit)) ? 20.0f : 0.0f;

        if (lift > 0 && isMyTurn)
        {
            hoveredCard = i;
            Rectangle glowRect = { pos.x - 2, pos.y - 2 - lift,
                                   (float)CARD_WIDTH + 4, (float)CARD_HEIGHT + 4 + lift };
            DrawRectangleRounded(glowRect, 0.3f, 10, Fade(GOLD_COLOR, 0.4f));
        }

        card_renderer::drawCard(me.hand[i], (int)pos.x, (int)(pos.y - lift), 1.0f);
    }

    // UNO button
    unoButtonEnabled = (isMyTurn && handSize == 2);
    if (unoButtonEnabled)
    {
        Rectangle ubtn = { (float)(SCREEN_W / 2 + 60), (float)(SCREEN_H - 80), 100, 40 };
        Color unoRed = { 237, 28, 36, 255 };
        Color ucol = unoRed;
        if (CheckCollisionPointRec(m, ubtn)) ucol = Fade(ucol, 0.7f);
        DrawRectangleRounded(ubtn, 0.3f, 10, ucol);
        DrawRectangleRoundedLines(ubtn, 0.3f, 10, 2, GOLD_COLOR);
        int tw3 = MeasureText("UNO!", 22);
        DrawText("UNO!", (SCREEN_W - tw3) / 2, SCREEN_H - 74, 22, GOLD_COLOR);

        if (CheckCollisionPointRec(m, ubtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            pendingResult.action = PlayerAction::SAY_UNO;
            pendingResult.targetId = localPlayerId;
        }
    }

    // Catch targets
    vulnerableOpponent = -1;
    for (int i = 0; i < n; i++)
    {
        if (i == localPlayerId) continue;
        if ((int)state.players[i].hand.size() == 1)
        {
            vulnerableOpponent = i;
            break;
        }
    }
    if (vulnerableOpponent >= 0)
    {
        oppIdx = 0;
        for (int i = 0; i < n; i++)
        {
            if (i == localPlayerId) continue;
            if (i == vulnerableOpponent)
            {
                int sx = startX + oppIdx * (slotW + gap);
                Rectangle cr = { (float)sx, 110, (float)slotW, 30 };
                Color catchBg = { 237, 28, 36, 200 };
                DrawRectangleRounded(cr, 0.3f, 10, catchBg);
                int ctw = MeasureText("CATCH UNO?", 16);
                DrawText("CATCH UNO?", sx + (slotW - ctw) / 2, 114, 16, GOLD_COLOR);
            }
            oppIdx++;
        }
    }

    if (isMyTurn)
    {
        Rectangle btn = { (float)(SCREEN_W / 2 - 50), (float)(SCREEN_H - 75), 100, 30 };
        Color drawBtnHover = { 200, 180, 0, 255 };
        Color drawBtnNormal = { 180, 140, 20, 255 };
        if (CheckCollisionPointRec(m, btn))
            DrawRectangleRounded(btn, 0.3f, 10, drawBtnHover);
        else
            DrawRectangleRounded(btn, 0.3f, 10, drawBtnNormal);
        int tw4 = MeasureText("DRAW", 18);
        DrawText("DRAW", (SCREEN_W - tw4) / 2, SCREEN_H - 72, 18, WHITE);
        if (CheckCollisionPointRec(m, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            pendingResult.action = PlayerAction::DRAW_CARD;
    }

    if (needsColorPick)
        renderColorPicker();

    if (overlayTimer > 0)
        renderMessageOverlay();

    ParticleSystem::instance().update(GetFrameTime());
    ParticleSystem::instance().render();

    AnimationManager::instance().update(GetFrameTime());

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        if (vulnerableOpponent >= 0)
        {
            oppIdx = 0;
            for (int i = 0; i < n; i++)
            {
                if (i == localPlayerId) continue;
                if (i == vulnerableOpponent)
                {
                    int sx = startX + oppIdx * (slotW + gap);
                    Rectangle cr = { (float)sx, 20, (float)slotW, (float)slotH };
                    if (CheckCollisionPointRec(m, cr))
                    {
                        pendingResult.action = PlayerAction::CATCH_UNO;
                        pendingResult.targetId = i;
                        return;
                    }
                }
                oppIdx++;
            }
        }

        if (isMyTurn)
        {
            for (int i = 0; i < handSize; i++)
            {
                Vector2 pos = getHandCardPos(i, handSize);
                Rectangle cardArea = { pos.x, pos.y, (float)CARD_WIDTH, (float)CARD_HEIGHT };
                if (CheckCollisionPointRec(m, cardArea))
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
}
#endif
