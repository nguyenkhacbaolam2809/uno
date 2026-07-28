# Architecture

## Overview

UNO is a multiplayer card game implemented in C++17 with Raylib for rendering and epoll for Linux networking.

```
                ┌─────────────────────────────────────────────┐
                │                  main.cpp                   │
                └──────────────┬──────────────────────────────┘
                               │
                ┌──────────────▼──────────────────────────────┐
                │                Gui / State Machine           │
                │         (src/ui/gui.cpp + engine/)           │
                └──────┬──────────────┬────────────────┬───────┘
                       │              │                │
          ┌────────────▼───┐  ┌───────▼────────┐  ┌───▼────────────┐
          │   MenuState    │  │  PlayingState   │  │  GameOverState │
          │  (menu_view)   │  │  (game_view)    │  │                │
          └────────────────┘  └───────┬────────┘  └────────────────┘
                                      │
                     ┌────────────────┼────────────────┐
                     │                │                │
              ┌──────▼───────┐ ┌──────▼───────┐ ┌──────▼───────┐
              │  GameEngine  │ │  GameView    │ │  AudioManager│
              │  (core logic)│ │  (rendering) │ │  (sounds)    │
              └──────┬───────┘ └──────┬───────┘ └──────────────┘
                     │                │
              ┌──────▼───────┐ ┌──────▼──────────────────────┐
              │  Card/Deck/  │ │  GameRenderBoard/Cards/     │
              │  Player/Rules│ │  Players/UI/Effects          │
              └──────────────┘ └─────────────────────────────┘
```

## Directory Structure

```
src/
├── core/          # Game model
│   ├── card.h/cpp       # Card data type
│   ├── deck.h/cpp       # Deck of cards
│   ├── player.h/cpp     # Player state
│   ├── rules.h/cpp      # Game rules engine
│   ├── game_engine.h/cpp # Game loop & state mgmt
│   ├── config.h/cpp     # OS/lang config
│   └── rng.h            # MT19937 RNG singleton
├── ai/            # Bot strategies
│   ├── ibot_strategy.h  # Strategy interface
│   ├── bot_easy/cpp
│   ├── bot_medium/cpp
│   ├── bot_hard/cpp
│   └── bot_factory.h/cpp
├── network/       # LAN multiplayer
│   ├── packets.h        # Packet type definitions
│   ├── tcp_buffer.h     # Send/recv ring buffers
│   ├── network_server.h/cpp
│   └── network_client.h/cpp
├── ui/            # Rendering
│   ├── colors.h         # Color palette constants
│   ├── card_renderer.h/cpp
│   ├── game_view.h/cpp  # Main view orchestrator
│   ├── game_render_board.cpp
│   ├── game_render_cards.cpp
│   ├── game_render_players.cpp
│   ├── game_render_ui.cpp
│   ├── game_render_effects.cpp
│   ├── menu_view.h/cpp  # Menu system
│   └── gui.h/cpp        # Entry point
├── engine/        # State machine
│   └── game_state.h/cpp
├── utils/         # Infrastructure
│   ├── result.h         # Result<T> error handling
│   ├── logger.h/cpp     # Logging system
│   ├── input_manager.h/cpp
│   ├── asset_manager.h/cpp
│   ├── animation_manager.h/cpp
│   ├── particle_system.h/cpp
│   ├── debug_overlay.h/cpp
│   ├── audio_manager.h/cpp
│   └── settings.h       # Persistent config
└── tests/
    └── test_all.cpp     # Unit tests
```

## Rendering Pipeline

1. `Gui::run()` → `AppStateMachine::run()` → `PlayingState::render()`
2. `GameView::render()` orchestrates:
   - `renderBackground()` — green felt + dots
   - `renderOpponents()` — card-count badges + names
   - `renderPiles()` — draw pile + discard pile + direction
   - `renderHand()` — player's fanned cards with hover lift
   - `renderTurnIndicator()` — DRAW button
   - `renderUnoButton()` — UNO declaration button
   - `renderCatchTargets()` — catch UNO labels
   - `renderColorPicker()` — wild color overlay
   - `renderMessageOverlay()` — floating messages
3. `ParticleSystem::update()/render()` — overlaid particles
4. `AnimationManager::update()` — delta-time animations

## Networking Flow (LAN)

```
Server                          Client
  │                               │
  ├─ waitForPlayers() ───────────►│ connect()
  │                               │
  ├─ runGameLoop()                │
  │  ┌───────────────────────┐    │
  │  │ epoll_wait()          │    │
  │  │ handleRead()          │◄───┤ sendPlayCard()
  │  │ broadcastSyncState()──┼───►│ receiveSyncState()
  │  │ handleWrite()         │    │
  │  └───────────────────────┘    │
  │                               │
  └─ game over ───────────────────┘
```

## Animation System

```
AnimationManager (singleton)
  ├─ FloatAnim    — smooth numeric interpolation
  ├─ Vec2Anim     — position animation
  ├─ ColorAnim    — color transitions
  ├─ ShakeAnim    — screen shake
  ├─ DelayAnim    — timed callback
  ├─ SequenceAnim — serial composition
  └─ ParallelAnim — parallel composition
```

Easing functions: LINEAR, EASE_IN_QUAD, EASE_OUT_QUAD, EASE_IN_OUT_QUAD, EASE_OUT_BACK, EASE_OUT_ELASTIC, EASE_OUT_BOUNCE.
