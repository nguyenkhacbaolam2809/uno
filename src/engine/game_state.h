#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "raylib.h"
#include "config.h"
#include "game_engine.h"
#include "game_view.h"
#include "menu_view.h"
#include <memory>
#include <vector>
#include <string>

enum class AppStateId {
    MENU,
    LOBBY,
    PLAYING_LOCAL,
    GAME_OVER,
    EXIT
};

class IAppState {
public:
    virtual ~IAppState() = default;
    virtual AppStateId update() = 0;
    virtual void render() = 0;
    virtual AppStateId id() const = 0;
    virtual void enter() {}
    virtual void exit() {}
};

class MenuState : public IAppState {
public:
    explicit MenuState(const GameConfig & cfg);
    AppStateId update() override;
    void render() override;
    AppStateId id() const override { return AppStateId::MENU; }
    MenuResult getResult() const { return m_result; }

private:
    std::unique_ptr<MenuView> m_menuView;
    MenuResult m_result;
};

struct LobbyConfig {
    int numHumans{1};
    int numBots{1};
    int botDifficulty{1};
    bool vietRules{false};
    bool isLanServer{false};
    bool isLanClient{false};
    std::string serverIp{"127.0.0.1"};
    int port{8888};
};

struct GameOverInfo {
    int winner;
    int localPlayerId;
    std::vector<std::string> playerNames;
};

class PlayingState : public IAppState {
public:
    PlayingState(const GameConfig & cfg, const LobbyConfig & lobby);
    ~PlayingState() override;
    AppStateId update() override;
    void render() override;
    AppStateId id() const override { return AppStateId::PLAYING_LOCAL; }
    void enter() override;
    GameOverInfo getGameOverInfo() const;

private:
    GameConfig m_config;
    LobbyConfig m_lobby;
    GameEngine m_engine;
    std::unique_ptr<GameView> m_gameView;
    int m_localPlayerId{0};
    bool m_entered{false};

    void processTurnLocal();
};

class GameOverState : public IAppState {
public:
    explicit GameOverState(const GameOverInfo & info);
    AppStateId update() override;
    void render() override;
    AppStateId id() const override { return AppStateId::GAME_OVER; }
    void enter() override;
private:
    GameOverInfo m_info;
    bool m_entered{false};
};

class LobbyState : public IAppState {
public:
    LobbyState(const GameConfig & cfg, const LobbyConfig & lobby);
    AppStateId update() override;
    void render() override;
    AppStateId id() const override { return AppStateId::LOBBY; }
private:
    GameConfig m_config;
    LobbyConfig m_lobby;
    float m_timer{0};
};

class AppStateMachine {
public:
    AppStateMachine(const GameConfig & cfg);

    void run();

    void transitionTo(AppStateId newId);
    AppStateId currentId() const { return m_currentId; }

private:
    GameConfig m_config;
    AppStateId m_currentId{AppStateId::MENU};
    std::unique_ptr<IAppState> m_currentState;
    LobbyConfig m_lobbyConfig;
    GameOverInfo m_gameOverInfo;

    void createState(AppStateId id);
    void extractLobbyFromMenu();
    void extractGameOverFromPlaying();
};

#endif
