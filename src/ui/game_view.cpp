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
    : hoveredCard(-1), selectedCard(-1),
      pendingResult{PlayerAction::NONE, -1, -1, CardColor::Wild},
      needsColorPick(false), pickedColor(CardColor::Wild),
      overlayTimer(0.0f), unoButtonEnabled(false)
{
}

void GameView::resetInteraction()
{
    pendingResult.action = PlayerAction::NONE;
    pendingResult.cardIndex = -1;
    pendingResult.targetId = -1;
    pendingResult.chosenColor = CardColor::Wild;
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

void GameView::render(const GameEngine & engine, int localPlayerId)
{
    renderBackground();
    if (engine.getPlayerCount() <= 0 ||
        localPlayerId < 0 || localPlayerId >= engine.getPlayerCount())
        return;

    bool colorPickerWasOpen = needsColorPick;

    renderOpponents(engine, localPlayerId);
    renderPiles(engine);
    renderHand(engine, localPlayerId);
    renderTurnIndicator(engine, localPlayerId);
    renderUnoButton(engine, localPlayerId);
    renderCatchTargets(engine, localPlayerId);
    renderJumpInButton(engine, localPlayerId);

    if (engine.getPhase() == GamePhase::GameOver)
        renderWinConfetti(engine);

    if (needsColorPick)
        renderColorPicker();

    if (overlayTimer > 0)
        renderMessageOverlay();

    ParticleSystem::instance().update(GetFrameTime());
    ParticleSystem::instance().render();

    if (!colorPickerWasOpen)
    {
        handleHandClick(engine, localPlayerId);
        handleUnoCatchClick(engine, localPlayerId);
    }
}

#ifndef _WIN32
void GameView::renderSync(const SyncState & state, int localPlayerId)
{
    renderBackground();

    int n = (int)state.players.size();
    if (n <= 0 || localPlayerId < 0 || localPlayerId >= n)
        return;

    bool colorPickerWasOpen = needsColorPick;

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
        int cc = (int)state.players[i].hand.size();
        bool isCurrent = (state.gs.turn % n) == i;
        drawOpponentSlot(x, 20, slotW, slotH, state.players[i].name, cc, isCurrent, cc == 1);
        oppIdx++;
    }

    if (state.gs.phase == GamePhase::GameOver)
    {
        int winner = state.gs.winner;
        if (winner >= 0 && winner < n)
            drawGameOverText(state.players[winner].name + " wins!");
        return;
    }

    int cx = SCREEN_W / 2, cy = SCREEN_H / 2 - 20;
    int pileW = (int)(CARD_WIDTH * 1.1f);
    int pileH = (int)(CARD_HEIGHT * 1.1f);

    drawPileStack(cx, cy, pileW, pileH);
    drawDiscardPile(cx, cy, pileW, pileH, state.gs.currentCard);
    drawDirectionIndicator(cx, cy, pileH, state.gs.direction);
    drawForceDrawText(cx, cy, pileH, state.gs.forceDraw ? state.gs.drawStack : 0);

    const SyncPlayer & me = state.players[localPlayerId];
    int handSize = (int)me.hand.size();
    bool isMyTurn = (state.gs.turn % n) == localPlayerId;
    Vector2 m = GetMousePosition();

    hoveredCard = -1;
    if (isMyTurn)
    {
        for (int i = handSize - 1; i >= 0; i--)
        {
            Vector2 pos = getHandCardPos(i, handSize);
            Rectangle cardHit = { pos.x, pos.y, (float)CARD_WIDTH, (float)CARD_HEIGHT };
            if (CheckCollisionPointRec(m, cardHit))
            {
                hoveredCard = i;
                break;
            }
        }
    }

    for (int i = 0; i < handSize; i++)
    {
        Vector2 pos = getHandCardPos(i, handSize);
        float lift = (i == hoveredCard) ? 20.0f : 0.0f;

        if (i == hoveredCard)
        {
            Rectangle glowRect = { pos.x - 2, pos.y - 2 - lift,
                                   (float)CARD_WIDTH + 4, (float)CARD_HEIGHT + 4 + lift };
            DrawRectangleRounded(glowRect, 0.3f, 10, Fade(GOLD_COLOR, 0.4f));
        }

        card_renderer::drawCard(me.hand[i], (int)pos.x, (int)(pos.y - lift), 1.0f);
    }

    // UNO button
    unoButtonEnabled = (!needsColorPick && isMyTurn && handSize == 2);
    if (unoButtonEnabled)
    {
        Rectangle ubtn = { (float)(SCREEN_W / 2 + 140), (float)(SCREEN_H - CARD_HEIGHT - 100), 100, 40 };
        Color unoRed = { 237, 28, 36, 255 };
        Color ucol = unoRed;
        if (CheckCollisionPointRec(m, ubtn)) ucol = Fade(ucol, 0.7f);
        DrawRectangleRounded(ubtn, 0.3f, 10, ucol);
        DrawRectangleRoundedLines(ubtn, 0.3f, 10, 2, GOLD_COLOR);
        int tw3 = MeasureText("UNO!", 22);
        DrawText("UNO!", (int)(ubtn.x + (ubtn.width - tw3) / 2),
                 (int)(ubtn.y + (ubtn.height - 22) / 2), 22, GOLD_COLOR);

        if (CheckCollisionPointRec(m, ubtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            pendingResult.action = PlayerAction::SAY_UNO;
            pendingResult.targetId = localPlayerId;
        }
    }

    // Catch targets (LAN sync has no UNO-called flag, so every 1-card
    // opponent is shown; the server still validates the accusation).
    m_catchableOpponents.clear();
    if (state.gs.vietRules)
    {
        for (int i = 0; i < n; i++)
            if (i != localPlayerId && (int)state.players[i].hand.size() == 1)
                m_catchableOpponents.push_back(i);
    }

    if (!m_catchableOpponents.empty())
    {
        oppIdx = 0;
        for (int i = 0; i < n; i++)
        {
            if (i == localPlayerId) continue;
            if (std::find(m_catchableOpponents.begin(), m_catchableOpponents.end(), i) !=
                m_catchableOpponents.end())
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

    if (isMyTurn && !needsColorPick)
    {
        Rectangle btn = { (float)(SCREEN_W - 180), (float)(SCREEN_H - CARD_HEIGHT - 100), 140, 40 };
        Color drawBtnHover = { 200, 180, 0, 255 };
        Color drawBtnNormal = { 180, 140, 20, 255 };
        if (CheckCollisionPointRec(m, btn))
            DrawRectangleRounded(btn, 0.3f, 10, drawBtnHover);
        else
            DrawRectangleRounded(btn, 0.3f, 10, drawBtnNormal);
        int tw4 = MeasureText("DRAW CARD", 18);
        DrawText("DRAW CARD", (int)(btn.x + (btn.width - tw4) / 2),
                 (int)(btn.y + 10), 18, WHITE);
        if (CheckCollisionPointRec(m, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            pendingResult.action = PlayerAction::DRAW_CARD;
    }

    if (needsColorPick)
        renderColorPicker();

    if (overlayTimer > 0)
        renderMessageOverlay();

    ParticleSystem::instance().update(GetFrameTime());
    ParticleSystem::instance().render();

    if (!colorPickerWasOpen && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        if (!m_catchableOpponents.empty())
        {
            oppIdx = 0;
            for (int i = 0; i < n; i++)
            {
                if (i == localPlayerId) continue;
                if (std::find(m_catchableOpponents.begin(), m_catchableOpponents.end(), i) !=
                    m_catchableOpponents.end())
                {
                    int sx = startX + oppIdx * (slotW + gap);
                    Rectangle cr = { (float)sx, 20, (float)slotW, 120 };
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
            for (int i = handSize - 1; i >= 0; i--)
            {
                Vector2 pos = getHandCardPos(i, handSize);
                Rectangle cardArea = { pos.x, pos.y, (float)CARD_WIDTH, (float)CARD_HEIGHT };
                if (CheckCollisionPointRec(m, cardArea))
                {
                    Card chosen = me.hand[i];
                    if (canPlayCard(chosen, state.gs.currentCard))
                    {
                        if (chosen.color == CardColor::Wild)
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
