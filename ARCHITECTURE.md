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

## Network Protocol

### Packet Framing
All packets use length-prefixed TCP framing:

```
[4 bytes: total length (network order)]
[1 byte: packet type]
[1 byte: player ID]
[N bytes: body]
```

### Packet Types

| Type | Value | Body | Direction |
|---|---|---|---|
| PKT_HEARTBEAT | 0 | PacketVersion | Both |
| PKT_PLAY_CARD | 1 | PacketPlayCard | Client→Server |
| PKT_DRAW | 2 | empty | Client→Server |
| PKT_JUMP_IN | 3 | PacketJumpIn | Client→Server |
| PKT_CALL_UNO | 4 | empty | Client→Server |
| PKT_CATCH_UNO | 5 | PacketUno | Client→Server |
| PKT_SYNC_STATE | 6 | serialized GameState | Server→Client |

### Protocol Version
Current version: `1` (defined in `packets.h` as `PROTOCOL_VERSION`).
The first packet from a client must include version information via
`PKT_HEARTBEAT` with a `PacketVersion` body. Servers reject mismatched
versions with a disconnection.

### SyncState Serialization
```
[int: myPlayerId]
[GameState: gs]
[int: playerCount]
  for each player:
    [int: nameLen][char[]: name]
    [int: cardCount][int: type][int: difficulty]
    [card[]: hand]
```

## Class Relationships

```
┌──────────────┐     owns      ┌──────────────────┐
│  GameEngine  │ ────────────► │     player[]     │
│              │               │  ┌────────────┐  │
│  ┌────────┐  │               │  │  card[]    │  │
│  │  deck  │  │               │  └────────────┘  │
│  └────────┘  │               └──────────────────┘
│  ┌────────┐  │
│  │  state │  │     uses      ┌──────────────────┐
│  └────────┘  │ ────────────► │  IBotStrategy   │
│  ┌────────┐  │               │  (polymorphic)  │
│  │discard │  │               └──────────────────┘
│  └────────┘  │
└──────────────┘
       │
       │ creates
       ▼
┌──────────────┐
│  GameState   │
│  (plain data)│
└──────────────┘
```

## Coding Conventions

- **Language**: C++17 with strict flags (`-Wall -Wextra -Wpedantic -Wshadow -Werror`)
- **Indentation**: 4 spaces
- **Naming**: `snake_case` for functions/variables, `PascalCase` for classes
- **Members**: `m_` prefix for private members
- **Memory**: No raw `new`/`delete`; use `std::unique_ptr`, `std::vector`
- **Namespaces**: No `using namespace std`; use explicit `std::` prefixes
- **Enums**: `enum class` for new enumerations; underlying type specified
- **Headers**: Include guards (`#ifndef`/`#define`/`#endif`), not `#pragma once`
- **Const**: Mark member functions `const` where possible; use `constexpr` for constants
- **Noexcept**: Mark functions `noexcept` where the implementation permits
- **Error handling**: Use `Result<T>` or `std::optional` instead of sentinel values
