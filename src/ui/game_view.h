#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include "raylib.h"
#include "game_engine.h"
#include "card.h"
#include "colors.h"
#ifndef _WIN32
#include "network_client.h"
#endif
#include <vector>
#include <string>

// Shared rendering helpers used by both render() and renderSync()
void drawOpponentSlot(int x, int y, int slotW, int slotH,
                      const std::string & name, int cardCount,
                      bool isCurrentTurn, bool hasOneCard);
void drawGameOverText(const std::string & winnerText);
void drawPileStack(int cx, int cy, int pileW, int pileH);
void drawDiscardPile(int cx, int cy, int pileW, int pileH, const Card & currentCard);
void drawDirectionIndicator(int cx, int cy, int pileH, int direction);
void drawForceDrawText(int cx, int cy, int pileH, int drawStack);

enum class PlayerAction {
    NONE,
    PLAY_CARD,
    DRAW_CARD,
    SAY_UNO,
    CATCH_UNO,
    JUMP_IN,
    NONE_SKIP
};

struct InteractionResult {
    PlayerAction action;
    int cardIndex;
    int targetId;
    CardColor chosenColor;
};

class GameView {
public:
    GameView();

    void render(const GameEngine & engine, int localPlayerId);
#ifndef _WIN32
    void renderSync(const SyncState & state, int localPlayerId);
#endif
    InteractionResult getInteraction();

    void showMessage(const std::string & msg, float duration = 2.0f);
    bool isReady() const noexcept;

    void resetInteraction();

private:
    int hoveredCard;
    int selectedCard;
    InteractionResult pendingResult;
    bool needsColorPick;
    CardColor pickedColor;
    std::string overlayMsg;
    float overlayTimer;

    bool unoButtonEnabled;
    int vulnerableOpponent;
    int handScrollOffset;

    void renderBackground();
    void renderOpponents(const GameEngine & engine, int localPlayerId);
    void renderPiles(const GameEngine & engine);
    void renderHand(const GameEngine & engine, int localPlayerId);
    void renderTurnIndicator(const GameEngine & engine, int localPlayerId);
    void renderColorPicker();
    void renderMessageOverlay();
    void renderUnoButton(const GameEngine & engine, int localPlayerId);
    void renderCatchTargets(const GameEngine & engine, int localPlayerId);
    void renderWinConfetti(const GameEngine & engine);
    void renderCardGlow(const Card & c, int x, int y, float scale);

    void handleHandClick(const GameEngine & engine, int localPlayerId);
    void handleUnoCatchClick(const GameEngine & engine, int localPlayerId);
    int cardAtPos(Vector2 mouse, const GameEngine & engine, int localPlayerId) const;

    Vector2 getHandCardPos(int index, int total) const;
    Rectangle getCardRect(int index, int total) const;
};

#endif
