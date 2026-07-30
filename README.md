# gameuno — C++ UNO Card Game

A feature-rich UNO card game written in Modern C++17 with Raylib GUI, AI bots, and LAN multiplayer support.

## Features

- **Full UNO rules**: Skip, Reverse, Draw 2, Wild, Wild Draw 4 with stacking
- **AI opponents**: 3 difficulty levels (Easy/Medium/Hard) using Strategy pattern
- **Multiplayer**: Local hot-seat, Human+Bot mixed, LAN (Linux)
- **Vietnamese rules**: Jump-in, Catch UNO, no action/finish cards
- **Graphics**: Raylib-accelerated 2D with particles, animations, and effects
- **Network**: epoll-based authoritative server (Linux)

## Architecture

```
gameuno/
├── main.cpp                Entry point (GUI)
├── src/
│   ├── core/               Domain logic (Card, Player, Deck, Rules, GameEngine)
│   ├── ai/                 Bot strategies (Strategy pattern + Factory)
│   ├── engine/             App state machine (Menu, Playing, GameOver)
│   ├── ui/                 Raylib GUI (game_view, card_renderer, colors)
│   ├── network/            TCP networking (epoll, packet framing)
│   └── utils/              Logging, assets, input, audio, particles, animations
├── tests/test_all.cpp      Unit tests (80+)
├── CMakeLists.txt          CMake build (C++17, FetchContent raylib)
└── Makefile                Build wrapper
```

### Key Design Decisions

- **Strategy Pattern** for AI (`IBotStrategy` → `Easy/Medium/HardBotStrategy`)
- **State Machine** for app flow (`AppStateMachine` → `MenuState` → `PlayingState`)
- **MVC-like separation**: Engine (model) independent of GUI (view)
- **`enum class` everywhere**: `CardColor`, `GamePhase`, `BotAction`, `PlayerType`
- **Modern C++**: `[[nodiscard]]`, `noexcept`, `constexpr`, `unique_ptr`, MT19937 RNG

## Build

### Prerequisites

- **Linux**: GCC 9+ / Clang 10+, CMake 3.16+, Raylib (auto-fetched)
- **Windows (WSL)**: Same as Linux

### Build & Run

```bash
# Debug build with sanitizers
make build

# Run tests
make test

# Launch game
make run
```

Or with CMake directly:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --parallel $(nproc)
./test_all
./gameuno
```

### Release Build

```bash
make release
./build-release/gameuno
```

## Controls

| Action | Input |
|--------|-------|
| Play card | Click card in hand |
| Draw card | Click "DRAW" button |
| Call UNO | Click "UNO!" button (when 2 cards) |
| Catch UNO | Click opponent slot (when they have 1 card) |
| Choose color | Pick from overlay (when playing Wild) |
| Toggle debug | F3 |
| Quit | Close window / ESC |

## Rule Variations

### Standard Rules
- Draw 2 / Wild Draw 4: next player draws + loses turn
- Wild Draw 4: only playable if you have no matching color
- Can finish on Skip, Reverse, or number cards

### Vietnamese Rules (toggle in menu)
- **Jump-in**: play exact matching card out of turn
- **Catch UNO**: penalize player who forgets to call UNO (+2 cards)
- **No action/finish cards**: Draw 2, Wild, Wild Draw 4 cannot end the game

## Running Tests

```bash
make test
# or directly:
./build/test_all
```

The test suite covers: Card equality, Deck creation/shuffling, Player hand management, Game engine lifecycle, Bot turn execution, Rules validation, and Wild Draw 4 constraints.

## Project Status

See [CHANGELOG.md](CHANGELOG.md) for version history and [ARCHITECTURE.md](ARCHITECTURE.md) for detailed design docs.
