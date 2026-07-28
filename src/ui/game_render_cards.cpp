#include "game_view.h"
#include "card_renderer.h"
#include "rules.h"

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

int GameView::cardAtPos(Vector2 mouse, const GameEngine & engine, int localPlayerId) const
{
    const player * p = engine.getPlayer(localPlayerId);
    int handSize = p->get_size();
    for (int i = 0; i < handSize; i++)
    {
        Rectangle r = getCardRect(i, handSize);
        if (CheckCollisionPointRec(mouse, r))
            return i;
    }
    return -1;
}
