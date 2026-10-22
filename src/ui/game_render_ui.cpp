#include "game_view.h"
#include "card_renderer.h"
#include "rules.h"
#include "colors.h"
#include <algorithm>

using uno::SCREEN_W;
using uno::SCREEN_H;
using uno::CARD_WIDTH;
using uno::CARD_HEIGHT;
using uno::GOLD_COLOR;

void GameView::renderUnoButton(const GameEngine & engine, int localPlayerId)
{
    unoButtonEnabled = false;
    if (needsColorPick) return;
    if (engine.getPhase() != GamePhase::Play) return;

    int n = engine.getPlayerCount();
    if (n <= 0 || localPlayerId < 0 || localPlayerId >= n)
        return;

    int current = engine.getCurrentTurn() % n;
    const Player * p = engine.getPlayer(localPlayerId);
    int handSize = p->get_size();

    unoButtonEnabled = (current == localPlayerId && handSize == 2 &&
                        !engine.hasCalledUno(localPlayerId));
    if (!unoButtonEnabled) return;

    Rectangle btn = { (float)(SCREEN_W / 2 + 140), (float)(SCREEN_H - CARD_HEIGHT - 100), 100, 40 };
    Color baseColor = { 237, 28, 36, 255 };
    if (CheckCollisionPointRec(GetMousePosition(), btn))
        baseColor = Fade(baseColor, 0.7f);
    DrawRectangleRounded(btn, 0.3f, 10, baseColor);
    DrawRectangleRoundedLines(btn, 0.3f, 10, 2, GOLD_COLOR);
    int utw = MeasureText("UNO!", 22);
    DrawText("UNO!", (int)(btn.x + (btn.width - utw) / 2),
             (int)(btn.y + (btn.height - 22) / 2), 22, GOLD_COLOR);

    if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        pendingResult.action = PlayerAction::SAY_UNO;
        pendingResult.targetId = localPlayerId;
    }
}

void GameView::renderCatchTargets(const GameEngine & engine, int localPlayerId)
{
    m_catchableOpponents.clear();
    if (engine.getPhase() != GamePhase::Play) return;
    if (!engine.getState().vietRules) return;
    int n = engine.getPlayerCount();
    if (n <= 0 || localPlayerId < 0 || localPlayerId >= n)
        return;

    for (int i = 0; i < n; i++)
    {
        if (i == localPlayerId) continue;
        const Player * p = engine.getPlayer(i);
        // A player who already called UNO is protected. Showing the button
        // for them only baits the local player into a false-accusation penalty.
        if (p->get_size() == 1 && !engine.hasCalledUno(i))
            m_catchableOpponents.push_back(i);
    }

    if (m_catchableOpponents.empty()) return;

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
        if (std::find(m_catchableOpponents.begin(), m_catchableOpponents.end(), i) !=
            m_catchableOpponents.end())
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

int GameView::findJumpInCard(const GameEngine & engine, int localPlayerId) const
{
    int n = engine.getPlayerCount();
    if (n <= 0 || localPlayerId < 0 || localPlayerId >= n)
        return -1;

    const Player * p = engine.getPlayer(localPlayerId);
    Card current = engine.getCurrentCard();
    for (int i = 0; i < p->get_size(); i++)
    {
        Card c = p->peek(i);
        if (!canJumpIn(c, current)) continue;
        // The jump-in UI has no separate color picker; wild-on-wild is
        // handled by bots (which choose a color) but not offered to humans.
        if (c.color == CardColor::Wild) continue;
        if (p->get_size() == 1 && !isLegalLastCard(c)) continue;
        return i;
    }
    return -1;
}

void GameView::renderJumpInButton(const GameEngine & engine, int localPlayerId)
{
    if (needsColorPick) return;
    if (engine.getPhase() != GamePhase::Play) return;
    if (!engine.getState().vietRules) return;
    if (engine.getState().jumpState != JumpInState::Available) return;

    int n = engine.getPlayerCount();
    if (n <= 0 || localPlayerId < 0 || localPlayerId >= n) return;
    if ((engine.getCurrentTurn() % n) == localPlayerId) return;

    int cardIdx = findJumpInCard(engine, localPlayerId);
    if (cardIdx < 0) return;

    Rectangle btn = { (float)(SCREEN_W / 2 - 80), (float)(SCREEN_H - CARD_HEIGHT - 100),
                      160, 42 };
    Color base = { 0, 155, 72, 255 };
    if (CheckCollisionPointRec(GetMousePosition(), btn))
        base = Fade(base, 0.7f);
    DrawRectangleRounded(btn, 0.3f, 10, base);
    DrawRectangleRoundedLines(btn, 0.3f, 10, 2, GOLD_COLOR);

    const char * text = "JUMP IN!";
    int tw = MeasureText(text, 20);
    DrawText(text, (int)(btn.x + (btn.width - tw) / 2),
             (int)(btn.y + (btn.height - 20) / 2), 20, WHITE);

    if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Card c = engine.getPlayer(localPlayerId)->peek(cardIdx);
        pendingResult.action = PlayerAction::JUMP_IN;
        pendingResult.cardIndex = cardIdx;
        pendingResult.targetId = localPlayerId;
        pendingResult.chosenColor = c.color;
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

    bool picked = false;
    bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    for (int i = 0; i < 4; i++)
    {
        Rectangle r = { (float)(startX + i * (btnW + gap)), (float)startY, (float)btnW, (float)btnH };
        Color c = colors[i].c;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), r);
        if (hovered)
            c = Fade(c, 0.7f);
        DrawRectangleRounded(r, 0.3f, 10, c);

        int lw = MeasureText(colors[i].label, 16);
        DrawText(colors[i].label, (int)(r.x + (r.width - lw) / 2), (int)(r.y + (r.height - 20) / 2), 16, WHITE);

        if (hovered && clicked)
        {
            pickedColor = colors[i].col;
            pendingResult.action = PlayerAction::PLAY_CARD;
            pendingResult.cardIndex = selectedCard;
            pendingResult.chosenColor = pickedColor;
            needsColorPick = false;
            picked = true;
        }
    }

    if (!picked && (clicked || IsKeyPressed(KEY_ESCAPE)))
    {
        // Clicking outside the palette (or Escape) cancels the wild-card play.
        // Also discard any button interaction captured under the overlay this
        // frame, otherwise the cancelled pick could turn into a draw/UNO click.
        needsColorPick = false;
        selectedCard = -1;
        pendingResult.action = PlayerAction::NONE;
        pendingResult.cardIndex = -1;
        pendingResult.targetId = -1;
        pendingResult.chosenColor = CardColor::Wild;
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
    if (engine.getPhase() != GamePhase::Play) return;
    if (needsColorPick) return;
    if (pendingResult.action != PlayerAction::NONE) return;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;

    int n = engine.getPlayerCount();
    if (n <= 0 || localPlayerId < 0 || localPlayerId >= n)
        return;

    bool isMyTurn = (engine.getCurrentTurn() % n) == localPlayerId;
    if (!isMyTurn) return;

    const Player * p = engine.getPlayer(localPlayerId);
    int cardIndex = cardAtPos(GetMousePosition(), engine, localPlayerId);
    if (cardIndex < 0) return;

    Card c = p->peek(cardIndex);
    Card current = engine.getCurrentCard();

    if (!canPlayCard(c, current))
    {
        showMessage("That card does not match the pile.");
        return;
    }

    if (!engine.validatePlay(localPlayerId, cardIndex))
    {
        if (engine.isForceDraw())
            showMessage("Stack a Draw 2 / Wild Draw 4 or draw a card.");
        else if (c.number == CARD_WILD_DRAW_FOUR)
            showMessage("Wild Draw 4 can only be played if you have no card of the current color.");
        else if (engine.getState().vietRules && p->get_size() == 1)
            showMessage("That card cannot be your last card.");
        else
            showMessage("You cannot play that card.");
        return;
    }

    if (c.color == CardColor::Wild)
    {
        needsColorPick = true;
        selectedCard = cardIndex;
    }
    else
    {
        pendingResult.action = PlayerAction::PLAY_CARD;
        pendingResult.cardIndex = cardIndex;
        pendingResult.chosenColor = c.color;
    }
}

void GameView::handleUnoCatchClick(const GameEngine & engine, int localPlayerId)
{
    if (engine.getPhase() != GamePhase::Play) return;
    if (pendingResult.action != PlayerAction::NONE) return;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
    if (m_catchableOpponents.empty()) return;

    int n = engine.getPlayerCount();
    if (n <= 0 || localPlayerId < 0 || localPlayerId >= n)
        return;

    int slotW = 180;
    int gap = 20;
    int others = n - 1;
    int totalW = others * slotW + (others - 1) * gap;
    int startX = (SCREEN_W - totalW) / 2;
    int oppIdx = 0;
    for (int i = 0; i < n; i++)
    {
        if (i == localPlayerId) continue;
        if (std::find(m_catchableOpponents.begin(), m_catchableOpponents.end(), i) !=
            m_catchableOpponents.end())
        {
            int x = startX + oppIdx * (slotW + gap);
            // Include the visible "CATCH UNO?" bar below the player slot.
            Rectangle r = { (float)x, 20, (float)slotW, 120 };
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
