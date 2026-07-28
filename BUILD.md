# Build Guide

## Prerequisites

- **Compiler**: GCC 9+ or Clang 10+ with C++17 support
- **CMake**: 3.16+
- **Raylib**: Optional — fetched automatically via CMake FetchContent
- **OS**: Linux with epoll support (WSL works)

## Quick Start

```bash
git clone https://github.com/nguyenkhacbaolam2809/uno.git
cd uno
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Build Options

| Option | Default | Description |
|---|---|---|
| `ENABLE_SANITIZERS` | ON | AddressSanitizer + UndefinedBehaviorSanitizer |
| `BUILD_TESTS` | ON | Build the test suite |

Example with sanitizers disabled:
```bash
cmake .. -DENABLE_SANITIZERS=OFF
```

## Running

```bash
# Run the game
./gameuno

# Run unit tests
./test_all
```

## Project Structure

- `src/core/` — Game model (card, deck, player, rules, engine)
- `src/ai/` — Bot strategies (easy/medium/hard)
- `src/network/` — LAN multiplayer (epoll-based)
- `src/ui/` — Raylib rendering (cards, game view, menus)
- `src/engine/` — State machine (menu, playing, game over)
- `src/utils/` — Infrastructure (logger, settings, audio, animations)
- `tests/` — Unit tests

## Dependencies

- **Raylib 5.0** — Rendering, input, audio (auto-fetched)
- **Linux epoll** — Network I/O (no Windows support)

## Code Quality

The project compiles with:
- `-Wall -Wextra -Wpedantic -Wshadow -Werror`
- AddressSanitizer + UndefinedBehaviorSanitizer

## Troubleshooting

**"Failed to init audio device"** — Install ALSA/dev libraries:
```bash
sudo apt install libasound2-dev
```

**Raylib fetch fails** — Ensure network access or install Raylib system-wide:
```bash
sudo apt install libraylib-dev
```
