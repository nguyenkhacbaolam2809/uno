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
#include <cstdlib>

// --- MenuState ---
MenuState::MenuState(const GameConfig & cfg)
    : m_menuView(std::make_unique<MenuView>(cfg))
{
}

AppStateId MenuState::update()
{
    if (m_done) return AppStateId::EXIT;
    m_result = m_menuView->show();
    if (!m_result.confirmed || m_result.action == -1)
    {
        m_done = true;
        return AppStateId::EXIT;
    }
    return AppStateId::LOBBY;
}

void MenuState::render()
{
    // MenuView handles its own rendering in show()
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
    int total;
    if (m_lobby.numBots > 0 || m_lobby.numHumans > 1)
        total = std::max(m_lobby.numHumans + m_lobby.numBots, 2);
    else
        total = 2;

    m_engine.init(total);
    m_localPlayerId = 0;

    for (int i = 0; i < total; i++)
    {
        std::string name;
        PlayerType type = HUMAN;
        int diff = 0;

        if (m_lobby.numHumans > 1)
        {
            name = "Player " + std::to_string(i + 1);
            type = HUMAN;
        }
        else
        {
            if (i == 0)
            {
                name = "Player";
                type = HUMAN;
            }
            else
            {
                name = "Bot " + std::to_string(i);
                type = BOT;
                diff = m_lobby.botDifficulty - 1;
            }
        }

        m_engine.addPlayer(name, type, diff);
    }

    m_engine.start();
    LOG_INFO("Game started with %d players", total);
}

AppStateId PlayingState::update()
{
    if (WindowShouldClose()) return AppStateId::EXIT;

    if (m_engine.isGameOver())
    {
        LOG_INFO("Game over, winner: %d", m_engine.getWinner());
        return AppStateId::GAME_OVER;
    }

    processTurnLocal();
    return AppStateId::PLAYING_LOCAL;
}

void PlayingState::render()
{
    m_gameView->render(m_engine, m_localPlayerId);
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

static SoundId cardSound(const card & c)
{
    if (c.number == CARD_SKIP) return SoundId::SKIP;
    if (c.number == CARD_REVERSE) return SoundId::REVERSE;
    if (c.number == CARD_WILD || c.number == CARD_WILD_DRAW_FOUR) return SoundId::WILD_CHOOSE;
    if (c.number == CARD_DRAW_TWO) return SoundId::CARD_DRAW;
    return SoundId::CARD_SLIDE;
}

void PlayingState::processTurnLocal()
{
    int n = m_engine.getPlayerCount();
    int currentTurn = m_engine.getCurrentTurn() % n;
    const player * p = m_engine.getPlayer(currentTurn);

    if (p->isBot())
    {
        double waitUntil = GetTime() + 0.5;
        while (GetTime() < waitUntil)
        {
            if (WindowShouldClose()) return;
            BeginDrawing();
            m_gameView->render(m_engine, m_localPlayerId);
            EndDrawing();
        }

        BotActionResult act = m_engine.executeBotTurn(currentTurn);
        if (act.action == BOT_PLAY_CARD)
        {
            card c = p->peek(act.cardIdx);
            AudioManager::instance().playSound(cardSound(c));
        }
        m_engine.playCard(currentTurn, act.cardIdx, colorToString(act.chosenColor));
        m_engine.nextTurn();
    }
    else
    {
        m_gameView->resetInteraction();
        bool turnDone = false;
        bool drewCard = false;

        while (!turnDone && !m_engine.isGameOver())
        {
            if (WindowShouldClose()) return;
            BeginDrawing();
            m_gameView->render(m_engine, m_localPlayerId);

            InteractionResult r = m_gameView->getInteraction();
            if (r.action == PlayerAction::PLAY_CARD)
            {
                if (m_engine.validatePlay(m_localPlayerId, r.cardIndex))
                {
                    card c = m_engine.getPlayer(m_localPlayerId)->peek(r.cardIndex);
                    AudioManager::instance().playSound(cardSound(c));
                    m_engine.playCard(m_localPlayerId, r.cardIndex,
                                     colorToString(r.chosenColor));
                    drewCard = false;
                    m_engine.nextTurn();
                    turnDone = true;
                }
            }
            else if (r.action == PlayerAction::DRAW_CARD)
            {
                if (!drewCard)
                {
                    AudioManager::instance().playSound(SoundId::CARD_DRAW);
                    drewCard = true;
                }
                m_engine.drawCard(m_localPlayerId);
                m_engine.nextTurn();
                turnDone = true;
            }
            else if (r.action == PlayerAction::SAY_UNO)
            {
                AudioManager::instance().playSound(SoundId::UNO_BUTTON);
                m_engine.callUno(m_localPlayerId);
            }
            else if (r.action == PlayerAction::CATCH_UNO)
            {
                AudioManager::instance().playSound(SoundId::CATCH_UNO);
                m_engine.catchUno(m_localPlayerId, r.targetId);
            }

            EndDrawing();
        }
    }

    if (m_engine.isGameOver())
    {
        int winner = m_engine.getWinner();
        AudioManager::instance().playSound(winner == m_localPlayerId ? SoundId::WIN : SoundId::LOSE);
    }
}

// --- GameOverState ---
GameOverState::GameOverState(GameEngine & engine, std::unique_ptr<GameView> view)
    : m_engine(engine), m_view(std::move(view))
{
}

AppStateId GameOverState::update()
{
    if (m_done) return AppStateId::EXIT;
    m_view->showGameOver(m_engine);
    m_done = true;
    return AppStateId::EXIT;
}

void GameOverState::render()
{
    // GameOver handled internally by showGameOver loop
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

            if (m_currentId == AppStateId::PLAYING_LOCAL)
            {
                BeginDrawing();
                Color bgColor = { 30, 80, 40, 255 };
ClearBackground(bgColor);
                m_currentState->render();

                DebugOverlay::instance().update(GetFrameTime());
                DebugOverlay::instance().setInfo("State", "PlayingLocal");
                DebugOverlay::instance().setInfo("Anims", TextFormat("%d", AnimationManager::instance().activeCount()));
                DebugOverlay::instance().setInfo("Particles", TextFormat("%d", ParticleSystem::instance().activeCount()));
                DebugOverlay::instance().render();

                EndDrawing();
            }

            m_currentState->exit();
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

void AppStateMachine::transitionTo(AppStateId newId)
{
    m_currentId = newId;
}

void AppStateMachine::createState(AppStateId id)
{
    switch (id)
    {
        case AppStateId::MENU:
            m_currentState = std::make_unique<MenuState>(m_config);
            break;
        case AppStateId::LOBBY:
        {
            // Quick transition through lobby for now (inline config)
            m_lobbyConfig.numHumans = 1;
            m_lobbyConfig.numBots = 1;
            m_lobbyConfig.botDifficulty = 1;
            m_currentId = AppStateId::PLAYING_LOCAL;
            createState(AppStateId::PLAYING_LOCAL);
            break;
        }
        case AppStateId::PLAYING_LOCAL:
            m_currentState = std::make_unique<PlayingState>(m_config, m_lobbyConfig);
            break;
        case AppStateId::GAME_OVER:
            // Handled via PlayingState -> GameOverState transition
            break;
        default:
            m_currentState = nullptr;
            break;
    }
}
