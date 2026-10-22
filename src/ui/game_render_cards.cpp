#include "game_view.h"
#include "card_renderer.h"
#include "rules.h"
#include "colors.h"

using uno::SCREEN_W;
using uno::SCREEN_H;
using uno::CARD_WIDTH;
using uno::CARD_HEIGHT;
using uno::GOLD_COLOR;

void GameView::renderHand(const GameEngine & engine, int localPlayerId)
{
    if (engine.getPlayerCount() <= 0 ||
        localPlayerId < 0 || localPlayerId >= engine.getPlayerCount())
        return;

    const Player * p = engine.getPlayer(localPlayerId);
    int handSize = p->get_size();
    if (handSize == 0) return;

    bool isMyTurn = (engine.getCurrentTurn() % engine.getPlayerCount()) == localPlayerId;

    // Cards overlap and later cards are drawn on top of earlier ones. Use the
    // same top-most hit-test as the click handler so hovering and clicking
    // always refer to the card the player can actually see.
    hoveredCard = isMyTurn ? cardAtPos(GetMousePosition(), engine, localPlayerId) : -1;

    for (int i = 0; i < handSize; i++)
    {
        Vector2 pos = getHandCardPos(i, handSize);
        float lift = (i == hoveredCard) ? 20.0f : 0.0f;

        Card c = p->peek(i);

        if (i == hoveredCard)
            renderCardGlow(c, (int)pos.x, (int)(pos.y - lift), 1.0f);

        card_renderer::drawCard(c, (int)pos.x, (int)(pos.y - lift), 1.0f);
    }
}

Vector2 GameView::getHandCardPos(int index, int total) const
{
    float overlap = (total > 7) ? 50.0f : 70.0f;
    float totalW = overlap * (total - 1) + CARD_WIDTH;
    float startX = (SCREEN_W - totalW) / 2.0f;
    float baseY = (float)(SCREEN_H - CARD_HEIGHT - 30);
    return { startX + index * overlap, baseY };
}

Rectangle GameView::getCardRect(int index, int total) const
{
    Vector2 p = getHandCardPos(index, total);
    return { p.x, p.y, (float)CARD_WIDTH, (float)CARD_HEIGHT };
}

int GameView::cardAtPos(Vector2 mouse, const GameEngine & engine, int localPlayerId) const
{
    if (engine.getPlayerCount() <= 0 ||
        localPlayerId < 0 || localPlayerId >= engine.getPlayerCount())
        return -1;

    const Player * p = engine.getPlayer(localPlayerId);
    int handSize = p->get_size();
    // Later cards are drawn on top of earlier ones, so test from right to left.
    for (int i = handSize - 1; i >= 0; i--)
    {
        Rectangle r = getCardRect(i, handSize);
        if (CheckCollisionPointRec(mouse, r))
            return i;
    }
    return -1;
}
