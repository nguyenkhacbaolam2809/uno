#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include "raylib.h"
#include "game_engine.h"
#include "card.h"
#include "colors.h"
#include "network_client.h"
#include <vector>
#include <string>
#include <functional>

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
    COLOR chosenColor;
};

class GameView {
public:
    GameView();

    void render(const GameEngine & engine, int localPlayerId);
    void renderSync(const SyncState & state, int localPlayerId);
    InteractionResult getInteraction();

    void showMessage(const std::string & msg, float duration = 2.0f);
    void showGameOver(const GameEngine & engine);
    bool isReady() const;

    void resetInteraction();

private:
    int hoveredCard;
    int selectedCard;
    InteractionResult pendingResult;
    bool needsColorPick;
    COLOR pickedColor;
    bool showUnoButton;

    std::string overlayMsg;
    float overlayTimer;

    int catchTarget;

    void renderBackground();
    void renderOpponents(const GameEngine & engine, int localPlayerId);
    void renderPiles(const GameEngine & engine);
    void renderHand(const GameEngine & engine, int localPlayerId);
    void renderTurnIndicator(const GameEngine & engine, int localPlayerId);
    void renderColorPicker();
    void renderMessageOverlay();
    void renderGameOver(const GameEngine & engine);

    void handleHandClick(const GameEngine & engine, int localPlayerId);
    int cardAtPos(Vector2 mouse, int localPlayerId);

    Vector2 getHandCardPos(int index, int total);
    Rectangle getCardRect(int index, int total);
};

#endif
