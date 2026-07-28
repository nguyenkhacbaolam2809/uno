#include "gui.h"
#include "card_renderer.h"
#include "rules.h"
#include "bot_factory.h"
#include "network_server.h"
#include "network_client.h"
#include <cstdlib>
#include <ctime>

static void showError(const char * text)
{
    constexpr int W = 400, H = 120;
    int x = (SCREEN_W - W) / 2, y = (SCREEN_H - H) / 2;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BG_DARK);
        DrawRectangleRounded((Rectangle){ (float)x, (float)y, (float)W, (float)H },
                             0.3f, 10, (Color){ 0, 0, 0, 200 });
        int tw = MeasureText(text, 22);
        DrawText(text, (SCREEN_W - tw) / 2, y + 30, 22, (Color){ 255, 100, 100, 255 });
        const char * sub = "Press ENTER to continue";
        int sw = MeasureText(sub, 16);
        DrawText(sub, (SCREEN_W - sw) / 2, y + 75, 16, Fade(WHITE, 0.6f));
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            break;
        EndDrawing();
    }
}

static const char * colorToString(COLOR c)
{
    switch (c)
    {
        case red:    return "do";
        case green:  return "xanh la";
        case blue:   return "xanh duong";
        case yellow: return "vang";
        default:     return "do";
    }
}

Gui::Gui(const GameConfig & cfg) : config(cfg), vietRules(false), running(false)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "UNO - Multiplayer Card Game");
    SetTargetFPS(60);

    gameView = std::make_unique<GameView>();
    menuView = std::make_unique<MenuView>(config);
}

Gui::~Gui()
{
    CloseWindow();
}

void Gui::run()
{
    while (!WindowShouldClose())
    {
        MenuResult menuResult = menuView->show();
        if (!menuResult.confirmed || menuResult.action == -1)
            break;

        vietRules = menuResult.vietRules;

        switch (menuResult.action)
        {
            case 1: runSinglePlayer(menuResult); break;
            case 2: runLocalMultiplayer(menuResult); break;
            case 3: runMixed(menuResult); break;
            case 4: runLanServer(menuResult); break;
            case 5: runLanClient(menuResult); break;
            default: break;
        }
    }
}

void Gui::processLocalTurn(GameEngine & engine, int localPlayerId)
{
    int n = engine.getPlayerCount();
    bool gameOverDisplayed = false;

    while (!engine.isGameOver())
    {
        int currentTurn = engine.getCurrentTurn() % n;
        const player * p = engine.getPlayer(currentTurn);

        if (WindowShouldClose()) return;

        if (p->isBot())
        {
            double waitUntil = GetTime() + 0.5;
            while (GetTime() < waitUntil)
            {
                BeginDrawing();
                gameView->render(engine, localPlayerId);
                EndDrawing();
                if (WindowShouldClose()) return;
            }

            BotActionResult act = engine.executeBotTurn(currentTurn);
            engine.playCard(currentTurn, act.cardIdx, colorToString(act.chosenColor));
            if (engine.isGameOver()) break;
            engine.nextTurn();
        }
        else
        {
            gameView->resetInteraction();
            bool turnDone = false;

            while (!turnDone && !engine.isGameOver())
            {
                BeginDrawing();
                gameView->render(engine, localPlayerId);

                InteractionResult r = gameView->getInteraction();
                if (r.action == PlayerAction::PLAY_CARD)
                {
                    if (engine.validatePlay(localPlayerId, r.cardIndex))
                    {
                        engine.playCard(localPlayerId, r.cardIndex,
                                      colorToString(r.chosenColor));
                        if (engine.isGameOver()) break;
                        engine.nextTurn();
                        turnDone = true;
                    }
                }
                else if (r.action == PlayerAction::DRAW_CARD)
                {
                    engine.drawCard(localPlayerId);
                    if (engine.isGameOver()) { turnDone = true; break; }
                    engine.nextTurn();
                    turnDone = true;
                }

                EndDrawing();
                if (WindowShouldClose()) return;
            }
        }
    }

    if (engine.isGameOver())
    {
        gameView->showGameOver(engine);
    }
}

void Gui::runSinglePlayer(const MenuResult & m)
{
    int total = 1 + m.numBots;
    GameEngine engine(config, vietRules);
    engine.init(total);
    engine.addPlayer(m.playerName.empty() ? "Player" : m.playerName, HUMAN, 0);
    for (int i = 1; i < total; i++)
        engine.addPlayer("Bot " + std::to_string(i), BOT, m.botDifficulty - 1);
    engine.start();
    processLocalTurn(engine, 0);
}

void Gui::runLocalMultiplayer(const MenuResult & m)
{
    int total = m.numHumans;
    GameEngine engine(config, vietRules);
    engine.init(total);
    for (int i = 0; i < total; i++)
        engine.addPlayer("Player " + std::to_string(i + 1), HUMAN, 0);
    engine.start();
    processLocalTurn(engine, 0);
}

void Gui::runMixed(const MenuResult & m)
{
    int total = m.numHumans + m.numBots;
    GameEngine engine(config, vietRules);
    engine.init(total);
    for (int i = 0; i < m.numHumans; i++)
        engine.addPlayer("Player " + std::to_string(i + 1), HUMAN, 0);
    for (int i = 0; i < m.numBots; i++)
        engine.addPlayer("Bot " + std::to_string(i + 1), BOT, m.botDifficulty - 1);
    engine.start();
    processLocalTurn(engine, 0);
}

void Gui::runLanServer(const MenuResult & m)
{
    NetworkServer server(config, vietRules);
    if (!server.start(m.port))
    {
        showError("Failed to start server!");
        return;
    }

    int remotePlayers = m.numHumans - 1;
    if (remotePlayers < 1) remotePlayers = 1;

    if (!server.waitForPlayers(remotePlayers))
        return;

    server.runGameLoop();
}

void Gui::runLanClient(const MenuResult & m)
{
    NetworkClient client;
    if (!client.connect(m.serverIp, m.port))
    {
        showError("Failed to connect to server!");
        return;
    }

    SyncState state;
    int myPlayerId = 0;

    while (client.isConnected() && !WindowShouldClose())
    {
        if (!client.receiveSyncState(state, 50))
            continue;

        myPlayerId = state.myPlayerId;

        BeginDrawing();
        gameView->renderSync(state, myPlayerId);
        EndDrawing();

        if (state.gs.phase == PHASE_GAME_OVER)
            break;

        InteractionResult r = gameView->getInteraction();
        if (r.action == PlayerAction::PLAY_CARD)
            client.sendPlayCard(r.cardIndex, colorToString(r.chosenColor), myPlayerId);
        else if (r.action == PlayerAction::DRAW_CARD)
            client.sendDraw(myPlayerId);
    }

    client.disconnect();
}
