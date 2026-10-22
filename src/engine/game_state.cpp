#include "game_state.h"
#ifndef _WIN32
#include "network_server.h"
#include "network_client.h"
#endif
#include "bot_factory.h"
#include "rules.h"
#include "logger.h"
#include "input_manager.h"
#include "debug_overlay.h"
#include "audio_manager.h"
#include "animation_manager.h"
#include "particle_system.h"
#include "settings.h"
#include "rng.h"
#include <cstdlib>
#include <algorithm>

// --- MenuState ---
MenuState::MenuState(const GameConfig & cfg)
    : m_menuView(std::make_unique<MenuView>(cfg))
{
}

AppStateId MenuState::update()
{
    bool finished = m_menuView->step();
    if (!finished) return AppStateId::MENU;

    m_result = m_menuView->getStepResult();
    if (!m_result.confirmed || m_result.action == -1)
        return AppStateId::EXIT;
    return AppStateId::LOBBY;
}

void MenuState::render()
{
    m_menuView->drawCurrentMenu();
}

// --- PlayingState ---
PlayingState::PlayingState(const GameConfig & cfg, const LobbyConfig & lobby)
    : m_config(cfg), m_lobby(lobby), m_engine(cfg, lobby.vietRules),
      m_gameView(std::make_unique<GameView>())
{
}

PlayingState::~PlayingState() = default;

void PlayingState::enter()
{
    if (m_entered) return;
    m_entered = true;

    if (m_lobby.mode == GameMode::LanHost || m_lobby.mode == GameMode::LanClient)
    {
        LOG_WARN("%s", "LAN mode is not wired to the GUI; returning to the menu");
        return;
    }

    int humans = 1;
    int bots = 1;

    switch (m_lobby.mode)
    {
        case GameMode::Single:
            humans = 1;
            bots = std::clamp(m_lobby.numBots, 1, MAX_PLAYERS - 1);
            break;
        case GameMode::Local:
            humans = std::clamp(m_lobby.numHumans, 2, MAX_PLAYERS);
            bots = 0;
            break;
        case GameMode::Mixed:
            humans = std::clamp(m_lobby.numHumans, 1, MAX_PLAYERS - 1);
            bots = std::clamp(m_lobby.numBots, 0, MAX_PLAYERS - humans);
            if (humans + bots < 2)
                bots = 1;
            break;
        default:
            // Unknown/LAN modes must not silently fall back to a local game.
            LOG_WARN("%s", "Unsupported game mode; returning to the menu");
            return;
    }

    int total = std::clamp(humans + bots, 2, MAX_PLAYERS);
    if (humans > total)
        humans = total;
    bots = total - humans;

    m_engine.init(total);
    m_localPlayerId = 0;
    m_botTimer = -1.0;
    m_jumpWindow = false;

    int botDifficulty = std::clamp(m_lobby.botDifficulty - 1, 0, 2);

    for (int i = 0; i < total; i++)
    {
        bool isHuman = (i < humans);
        std::string name;
        PlayerType type = isHuman ? PlayerType::Human : PlayerType::Bot;

        if (isHuman)
        {
            if (m_lobby.mode == GameMode::Local)
                name = "Player " + std::to_string(i + 1);
            else if (i == 0)
                name = m_lobby.playerName.empty() ? "Player" : m_lobby.playerName;
            else
                name = "Player " + std::to_string(i + 1);
        }
        else
        {
            name = "Bot " + std::to_string(i - humans + 1);
        }

        m_engine.addPlayer(name, type, isHuman ? 0 : botDifficulty);
    }

    m_engine.start();
    LOG_INFO("Game started with %d players (%d humans, %d bots)", total, humans, bots);
}

AppStateId PlayingState::update()
{
    if (WindowShouldClose()) return AppStateId::EXIT;

    if (m_lobby.mode == GameMode::LanHost || m_lobby.mode == GameMode::LanClient)
        return AppStateId::MENU;
    if (m_engine.getPlayerCount() <= 0)
        return AppStateId::MENU;

    if (m_engine.isGameOver())
    {
        LOG_INFO("Game over, winner: %d", m_engine.getWinner());
        return AppStateId::GAME_OVER;
    }

    if (m_jumpWindow)
    {
        updateJumpWindow();
        return AppStateId::PLAYING_LOCAL;
    }

    processTurnLocal();
    return AppStateId::PLAYING_LOCAL;
}

GameOverInfo PlayingState::getGameOverInfo() const
{
    GameOverInfo info;
    info.winner = m_engine.getWinner();
    info.localPlayerId = m_localPlayerId;
    info.playerNames.clear();
    for (int i = 0; i < m_engine.getPlayerCount(); i++)
        info.playerNames.push_back(m_engine.getPlayer(i)->getName());
    return info;
}

void PlayingState::render()
{
    m_gameView->render(m_engine, m_localPlayerId);
}

static SoundId cardSound(const Card & c)
{
    if (c.number == CARD_SKIP) return SoundId::SKIP;
    if (c.number == CARD_REVERSE) return SoundId::REVERSE;
    if (c.number == CARD_WILD || c.number == CARD_WILD_DRAW_FOUR) return SoundId::WILD_CHOOSE;
    if (c.number == CARD_DRAW_TWO) return SoundId::CARD_DRAW;
    return SoundId::CARD_SLIDE;
}

static bool canPlayerJumpIn(const GameEngine & engine, int playerIdx, bool allowWild)
{
    int n = engine.getPlayerCount();
    if (playerIdx < 0 || playerIdx >= n) return false;

    const Player * p = engine.getPlayer(playerIdx);
    Card current = engine.getCurrentCard();
    for (int i = 0; i < p->get_size(); i++)
    {
        Card c = p->peek(i);
        if (!canJumpIn(c, current)) continue;
        if (!allowWild && c.color == CardColor::Wild) continue;
        if (p->get_size() == 1 && !isLegalLastCard(c)) continue;
        return true;
    }
    return false;
}

bool PlayingState::hasJumpCandidate() const
{
    int n = m_engine.getPlayerCount();
    if (n <= 0) return false;
    int current = m_engine.getCurrentTurn() % n;
    for (int i = 0; i < n; i++)
    {
        if (i == current) continue;
        bool allowWild = m_engine.getPlayer(i)->isBot();
        if (canPlayerJumpIn(m_engine, i, allowWild))
            return true;
    }
    return false;
}

void PlayingState::beginJumpWindow()
{
    int n = m_engine.getPlayerCount();
    if (n <= 0) return;
    int current = m_engine.getCurrentTurn() % n;
    double now = GetTime();

    m_jumpWindow = true;
    m_botTimer = -1.0;

    int humanCandidate = -1;
    for (int i = 0; i < n; i++)
    {
        if (i == current) continue;
        if (m_engine.getPlayer(i)->isBot()) continue;
        if (canPlayerJumpIn(m_engine, i, false))
        {
            humanCandidate = i;
            break;
        }
    }

    if (humanCandidate >= 0)
    {
        m_localPlayerId = humanCandidate;
        m_jumpDeadline = now + 1.5;
        // Give the human priority; bots act just before the deadline.
        m_jumpBotAt = now + 1.45;
    }
    else
    {
        m_jumpDeadline = now + 0.9;
        m_jumpBotAt = now + 0.25;
    }
}

void PlayingState::updateJumpWindow()
{
    int n = m_engine.getPlayerCount();
    if (n <= 0)
    {
        m_jumpWindow = false;
        return;
    }

    int current = m_engine.getCurrentTurn() % n;
    GameState state = m_engine.getState();
    if (!state.vietRules || state.jumpState != JumpInState::Available)
    {
        m_jumpWindow = false;
        if (!m_engine.isGameOver()) m_engine.nextTurn();
        return;
    }

    InteractionResult r = m_gameView->getInteraction();

    if (r.action == PlayerAction::JUMP_IN)
    {
        const Player * p = m_engine.getPlayer(m_localPlayerId);
        if (p && r.cardIndex >= 0 && r.cardIndex < p->get_size())
        {
            Card c = p->peek(r.cardIndex);
            if (m_engine.jumpIn(m_localPlayerId, r.cardIndex, r.chosenColor))
            {
                AudioManager::instance().playSound(cardSound(c));
                m_gameView->showMessage("Jump in!");
                m_jumpWindow = false;
                m_botTimer = -1.0;
                if (!m_engine.isGameOver()) m_engine.nextTurn();
                return;
            }
            m_gameView->showMessage("That card cannot jump in.");
        }
    }
    else if (r.action == PlayerAction::CATCH_UNO)
    {
        bool protectedTarget = m_engine.hasCalledUno(r.targetId);
        AudioManager::instance().playSound(SoundId::CATCH_UNO);
        m_engine.catchUno(m_localPlayerId, r.targetId);
        m_gameView->showMessage(protectedTarget
            ? "False accusation! You draw 2 cards."
            : "Caught! Target draws 2 cards.");
    }

    double now = GetTime();
    if (now >= m_jumpBotAt)
    {
        for (int i = 0; i < n; i++)
        {
            if (i == current) continue;
            const Player * p = m_engine.getPlayer(i);
            if (!p || !p->isBot()) continue;

            BotActionResult act = m_engine.executeBotJumpIn(i);
            if (act.action != BotAction::JumpIn)
                continue;

            Card c = p->peek(act.cardIdx);
            if (m_engine.jumpIn(i, act.cardIdx, act.chosenColor))
            {
                AudioManager::instance().playSound(cardSound(c));
                m_gameView->showMessage("Bot jumped in!");
                m_jumpWindow = false;
                m_botTimer = -1.0;
                if (!m_engine.isGameOver()) m_engine.nextTurn();
                return;
            }
        }
        m_jumpBotAt = now + 1000.0;
    }

    if (now >= m_jumpDeadline)
    {
        m_jumpWindow = false;
        m_botTimer = -1.0;
        if (!m_engine.isGameOver()) m_engine.nextTurn();
    }
}

void PlayingState::advanceAfterPlay()
{
    if (m_engine.isGameOver()) return;

    GameState state = m_engine.getState();
    if (state.vietRules && state.jumpState == JumpInState::Available && hasJumpCandidate())
        beginJumpWindow();
    else
        m_engine.nextTurn();
}

void PlayingState::processTurnLocal()
{
    int n = m_engine.getPlayerCount();
    if (n <= 0) return;
    int currentTurn = m_engine.getCurrentTurn() % n;
    const Player * p = m_engine.getPlayer(currentTurn);

    if (p->isBot())
    {
        // Catching UNO is allowed when it is not your turn. Consume any
        // pending click here so a catch during a bot turn is not lost (and
        // so stale hand clicks cannot fire on the next human turn).
        InteractionResult early = m_gameView->getInteraction();
        if (early.action == PlayerAction::CATCH_UNO &&
            m_engine.getState().vietRules)
        {
            bool protectedTarget = m_engine.hasCalledUno(early.targetId);
            AudioManager::instance().playSound(SoundId::CATCH_UNO);
            m_engine.catchUno(m_localPlayerId, early.targetId);
            m_gameView->showMessage(protectedTarget
                ? "False accusation! You draw 2 cards."
                : "Caught! Target draws 2 cards.");
        }

        double now = GetTime();
        if (m_botTimer < 0.0)
            m_botTimer = now + 0.5;

        if (now < m_botTimer)
            return;

        m_botTimer = -1.0;

        BotActionResult act = m_engine.executeBotTurn(currentTurn);

        if (act.action == BotAction::Draw)
        {
            m_engine.drawCard(currentTurn);
            m_engine.nextTurn();
        }
        else
        {
            if (act.action == BotAction::PlayCard || act.action == BotAction::StackCard)
            {
                Card c = p->peek(act.cardIdx);
                AudioManager::instance().playSound(cardSound(c));

                if (p->get_size() == 2)
                {
                    bool willCall = true;
                    if (p->getDifficulty() == BotDifficulty::Easy)
                        willCall = (randomInt(0, 1) == 0);
                    else if (p->getDifficulty() == BotDifficulty::Normal)
                        willCall = (randomInt(0, 4) != 0);

                    if (willCall)
                        m_engine.callUno(currentTurn);
                }
            }

            if (m_engine.playCard(currentTurn, act.cardIdx, act.chosenColor))
            {
                advanceAfterPlay();
            }
            else
            {
                m_engine.drawCard(currentTurn);
                m_engine.nextTurn();
            }
        }
    }
    else
    {
        m_localPlayerId = currentTurn;
        InteractionResult r = m_gameView->getInteraction();
        if (r.action == PlayerAction::PLAY_CARD)
        {
            if (m_engine.validatePlay(m_localPlayerId, r.cardIndex))
            {
                Card c = m_engine.getPlayer(m_localPlayerId)->peek(r.cardIndex);
                AudioManager::instance().playSound(cardSound(c));
                if (m_engine.playCard(m_localPlayerId, r.cardIndex, r.chosenColor))
                    advanceAfterPlay();
                else
                    m_gameView->showMessage("You cannot play that card.");
            }
            else
            {
                m_gameView->showMessage("You cannot play that card.");
            }
        }
        else if (r.action == PlayerAction::DRAW_CARD)
        {
            AudioManager::instance().playSound(SoundId::CARD_DRAW);
            m_engine.drawCard(m_localPlayerId);
            m_engine.nextTurn();
        }
        else if (r.action == PlayerAction::SAY_UNO)
        {
            if (m_engine.callUno(m_localPlayerId))
            {
                AudioManager::instance().playSound(SoundId::UNO_BUTTON);
                m_gameView->showMessage("UNO!");
            }
            else
            {
                m_gameView->showMessage("Call UNO when you have two cards on your turn.");
            }
        }
        else if (r.action == PlayerAction::CATCH_UNO)
        {
            if (m_engine.getState().vietRules)
            {
                bool protectedTarget = m_engine.hasCalledUno(r.targetId);
                AudioManager::instance().playSound(SoundId::CATCH_UNO);
                m_engine.catchUno(m_localPlayerId, r.targetId);
                m_gameView->showMessage(protectedTarget
                    ? "False accusation! You draw 2 cards."
                    : "Caught! Target draws 2 cards.");
            }
        }
    }
}

// --- GameOverState ---
GameOverState::GameOverState(const GameOverInfo & info)
    : m_info(info)
{
}

void GameOverState::enter()
{
    if (m_entered) return;
    m_entered = true;
    AudioManager::instance().playSound(
        m_info.winner == m_info.localPlayerId ? SoundId::WIN : SoundId::LOSE);

    ParticleBurstConfig cfg;
    cfg.origin = { uno::SCREEN_W / 2.0f, uno::SCREEN_H / 2.0f - 40.0f };
    cfg.count = 80;
    cfg.speedMin = 120;
    cfg.speedMax = 460;
    cfg.lifeMin = 1.0f;
    cfg.lifeMax = 2.8f;
    cfg.sizeMin = 3;
    cfg.sizeMax = 9;
    cfg.colorStart = uno::GOLD_COLOR;
    cfg.colorEnd = Fade(uno::GOLD_COLOR, 0);
    cfg.spread = 360;
    ParticleSystem::instance().burst(cfg);
}

AppStateId GameOverState::update()
{
    if (WindowShouldClose()) return AppStateId::EXIT;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return AppStateId::MENU;
    return AppStateId::GAME_OVER;
}

void GameOverState::render()
{
    std::string msg;
    if (m_info.winner >= 0 && m_info.winner < (int)m_info.playerNames.size())
        msg = m_info.playerNames[m_info.winner] + " wins!";
    else
        msg = "Game over!";

    Color bgColor = { 30, 80, 40, 255 };
    ClearBackground(bgColor);

    int tw = MeasureText(msg.c_str(), 48);
    DrawText(msg.c_str(), (uno::SCREEN_W - tw) / 2, uno::SCREEN_H / 2 - 60, 48, uno::GOLD_COLOR);

    const char * sub = "Press ENTER or click to continue";
    int sw = MeasureText(sub, 20);
    DrawText(sub, (uno::SCREEN_W - sw) / 2, uno::SCREEN_H / 2 + 20, 20, Fade(WHITE, 0.7f));

    Rectangle menuButton = {
        uno::SCREEN_W / 2.0f - 110.0f,
        uno::SCREEN_H / 2.0f + 60.0f,
        220.0f,
        48.0f
    };
    Color buttonColor = CheckCollisionPointRec(GetMousePosition(), menuButton)
                          ? Color{ 110, 110, 120, 255 }
                          : Color{ 80, 80, 90, 255 };
    DrawRectangleRounded(menuButton, 0.3f, 10, buttonColor);
    DrawRectangleRoundedLines(menuButton, 0.3f, 10, 2, Fade(WHITE, 0.3f));
    const char * buttonText = "Back to menu";
    int buttonWidth = MeasureText(buttonText, 20);
    DrawText(buttonText,
             static_cast<int>(menuButton.x + (menuButton.width - buttonWidth) / 2),
             static_cast<int>(menuButton.y + 13), 20, WHITE);

    ParticleSystem::instance().update(GetFrameTime());
    ParticleSystem::instance().render();
}

// --- LobbyState ---
LobbyState::LobbyState(const GameConfig & cfg, const LobbyConfig & lobby)
    : m_config(cfg), m_lobby(lobby)
{
}

AppStateId LobbyState::update()
{
    if (WindowShouldClose()) return AppStateId::EXIT;
    return AppStateId::PLAYING_LOCAL;
}

void LobbyState::render()
{
    Color bgColor = { 30, 30, 40, 255 };
    ClearBackground(bgColor);
    DrawText("Starting game...", uno::SCREEN_W / 2 - 80, uno::SCREEN_H / 2 - 10, 20, WHITE);
}

// --- AppStateMachine ---
AppStateMachine::AppStateMachine(const GameConfig & cfg) : m_config(cfg)
{
    Logger::instance().setLevel(LogLevel::DEBUG);
    Logger::instance().enableFileOutput("gameuno.log");

    Settings::instance().load();
    float masterVol = Settings::instance().getFloat("audio.master", 1.0f);
    float musicVol = Settings::instance().getFloat("audio.music", 0.8f);
    float effectsVol = Settings::instance().getFloat("audio.effects", 1.0f);
    AudioManager::instance().setMasterVolume(masterVol);
    AudioManager::instance().setMusicVolume(musicVol);
    AudioManager::instance().setEffectsVolume(effectsVol);
    if (Settings::instance().getBool("audio.muted", false))
        AudioManager::instance().setMuted(true);

    LOG_INFO("%s", "Game started");
}

void AppStateMachine::run()
{
    while (m_currentId != AppStateId::EXIT && !WindowShouldClose())
    {
        AudioManager::instance().update();
        AnimationManager::instance().update(GetFrameTime());
        InputManager::instance().update();

        if (IsKeyPressed(KEY_F3))
            DebugOverlay::instance().toggle();

        createState(m_currentId);

        if (m_currentState)
        {
            m_currentState->enter();
            AppStateId nextId = m_currentState->update();

            BeginDrawing();
            if (m_currentId == AppStateId::PLAYING_LOCAL)
            {
                Color bgColor = { 30, 80, 40, 255 };
                ClearBackground(bgColor);
                m_currentState->render();

                DebugOverlay::instance().update(GetFrameTime());
                DebugOverlay::instance().setInfo("State", "PlayingLocal");
            }
            else
            {
                m_currentState->render();
            }

            DebugOverlay::instance().setInfo("Anims", TextFormat("%d", AnimationManager::instance().activeCount()));
            DebugOverlay::instance().setInfo("Particles", TextFormat("%d", ParticleSystem::instance().activeCount()));
            DebugOverlay::instance().render();
            EndDrawing();

            m_currentState->exit();

            // Extract GameOverInfo from PlayingState before it's destroyed
            if (m_currentId == AppStateId::PLAYING_LOCAL && nextId == AppStateId::GAME_OVER)
                extractGameOverFromPlaying();

            // Extract lobby config from MenuState
            if (m_currentId == AppStateId::MENU && nextId == AppStateId::LOBBY)
                extractLobbyFromMenu();

            transitionTo(nextId);
        }
        else
        {
            break;
        }
    }

    Settings::instance().setFloat("audio.master", AudioManager::instance().masterVolume());
    Settings::instance().setFloat("audio.music", AudioManager::instance().musicVolume());
    Settings::instance().setFloat("audio.effects", AudioManager::instance().effectsVolume());
    Settings::instance().setBool("audio.muted", AudioManager::instance().isMuted());
    Settings::instance().save();
}

void AppStateMachine::extractLobbyFromMenu()
{
    MenuState * ms = dynamic_cast<MenuState*>(m_currentState.get());
    if (!ms) return;
    MenuResult r = ms->getResult();
    m_lobbyConfig.numHumans = r.numHumans;
    m_lobbyConfig.numBots = r.numBots;
    m_lobbyConfig.botDifficulty = r.botDifficulty;
    m_lobbyConfig.vietRules = r.vietRules;
    m_lobbyConfig.serverIp = r.serverIp;
    m_lobbyConfig.port = r.port;
    m_lobbyConfig.playerName = r.playerName.empty() ? "Player" : r.playerName;
    m_lobbyConfig.isLanServer = (r.action == 4);
    m_lobbyConfig.isLanClient = (r.action == 5);

    switch (r.action)
    {
        case 1: m_lobbyConfig.mode = GameMode::Single; break;
        case 2: m_lobbyConfig.mode = GameMode::Local; break;
        case 3: m_lobbyConfig.mode = GameMode::Mixed; break;
        case 4: m_lobbyConfig.mode = GameMode::LanHost; break;
        case 5: m_lobbyConfig.mode = GameMode::LanClient; break;
        default: m_lobbyConfig.mode = GameMode::Single; break;
    }
}

void AppStateMachine::extractGameOverFromPlaying()
{
    PlayingState * ps = dynamic_cast<PlayingState*>(m_currentState.get());
    if (!ps) return;
    m_gameOverInfo = ps->getGameOverInfo();
}

void AppStateMachine::transitionTo(AppStateId newId)
{
    m_currentId = newId;
}

void AppStateMachine::createState(AppStateId id)
{
    if (m_currentState && m_currentState->id() == id)
        return;

    switch (id)
    {
        case AppStateId::MENU:
            m_currentState = std::make_unique<MenuState>(m_config);
            break;
        case AppStateId::LOBBY:
            m_currentState = std::make_unique<LobbyState>(m_config, m_lobbyConfig);
            break;
        case AppStateId::PLAYING_LOCAL:
            m_currentState = std::make_unique<PlayingState>(m_config, m_lobbyConfig);
            break;
        case AppStateId::GAME_OVER:
            m_currentState = std::make_unique<GameOverState>(m_gameOverInfo);
            break;
        default:
            m_currentState = nullptr;
            break;
    }
}
