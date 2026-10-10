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
        PlayerType type = PlayerType::Human;
        int diff = 0;

        if (m_lobby.numHumans > 1)
        {
            name = "Player " + std::to_string(i + 1);
            type = PlayerType::Human;
        }
        else
        {
            if (i == 0)
            {
                name = m_lobby.playerName.empty() ? "Player" : m_lobby.playerName;
                type = PlayerType::Human;
            }
            else
            {
                name = "Bot " + std::to_string(i);
                type = PlayerType::Bot;
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

void PlayingState::processTurnLocal()
{
    int n = m_engine.getPlayerCount();
    if (n <= 0) return;
    int currentTurn = m_engine.getCurrentTurn() % n;
    const Player * p = m_engine.getPlayer(currentTurn);

    if (p->isBot())
    {
        static double botTimer = 0.0;

        double now = GetTime();
        if (botTimer == 0.0)
            botTimer = now + 0.5;

        if (now < botTimer)
            return;

        botTimer = 0.0;

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
                m_engine.nextTurn();
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
        InteractionResult r = m_gameView->getInteraction();
        if (r.action == PlayerAction::PLAY_CARD)
        {
            if (m_engine.validatePlay(m_localPlayerId, r.cardIndex))
            {
                Card c = m_engine.getPlayer(m_localPlayerId)->peek(r.cardIndex);
                AudioManager::instance().playSound(cardSound(c));
                if (m_engine.playCard(m_localPlayerId, r.cardIndex, r.chosenColor))
                {
                    m_engine.nextTurn();
                }
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
            AudioManager::instance().playSound(SoundId::UNO_BUTTON);
            m_engine.callUno(m_localPlayerId);
        }
        else if (r.action == PlayerAction::CATCH_UNO)
        {
            AudioManager::instance().playSound(SoundId::CATCH_UNO);
            m_engine.catchUno(m_localPlayerId, r.targetId);
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
}

AppStateId GameOverState::update()
{
    if (WindowShouldClose()) return AppStateId::EXIT;
    Rectangle menuButton = {
        uno::SCREEN_W / 2.0f - 110.0f,
        uno::SCREEN_H / 2.0f + 60.0f,
        220.0f,
        48.0f
    };
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
        (CheckCollisionPointRec(GetMousePosition(), menuButton) &&
         IsMouseButtonPressed(MOUSE_LEFT_BUTTON)))
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
