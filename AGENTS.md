# Project Progress

## Phase 1 - Foundation ✅
- Core data types: `card.h/cpp`, `deck.h/cpp`, `player.h/cpp`
- Game configuration: `config.h/cpp`
- Game logic: `rules.h/cpp`
- Singleplayer, local multiplayer, mixed (human+bot), LAN modes

## Phase 2 - AI Bot Engine with Strategy Pattern ✅
- `ibot_strategy.h` - Strategy interface with virtual methods
- `bot_easy.h/cpp` - Easy: plays first legal card or draws
- `bot_medium.h/cpp` - Medium: prioritizes action cards, wilds when lacking color
- `bot_hard.h/cpp` - Hard: strategic color counting, card value optimization, wild preference
- `bot_factory.h/cpp` - Factory to create bot strategies by difficulty
- `game_engine.h/cpp` - Updated with `executeBotTurn()`, `createBot()`, `BotActionResult`

## Phase 3 - Network Layer ✅
- `packets.h` - Packet types (PLAY_CARD, DRAW, JUMP_IN, CALL_UNO, CATCH_UNO, SYNC_STATE)
- `tcp_buffer.h` - Per-client send/recv ring buffers with length-prefixed TCP framing
- `network_server.h/cpp` - epoll-based authoritative server (Linux only, non-blocking I/O)
- `network_client.h/cpp` - epoll-based async client (Linux only, non-blocking I/O)

## Phase 4 - Testing & Polish ✅
- Comprehensive unit tests: 80 tests covering card, deck, player, rules, game engine, and bots
- Various bug fixes and edge case handling
- All 80 unit tests pass

## Phase 5 - Platform-Abstracted Network Layer ✅
- `net_platform.h` - Abstract `EventLoop` interface (epoll/select)
- `net_platform_epoll.cpp` - Linux epoll implementation
- `net_platform_win.cpp` - Windows select-based implementation
- `net_platform_shared.cpp` - `TcpReader`/`TcpWriter` with TCP fragmentation handling
- Rewrote `network_server.cpp` with single-threaded event loop
- Fixed TCP fragmentation with `TcpReader` two-stage read state machine
- Strict `-Wall -Werror -Wextra -Wshadow` compilation
- All 80 unit tests pass with zero warnings

## Phase 6 - 2D GUI & Modern Architecture ✅
- **File structure cleanup**: Removed `console_ui.*`, `net_platform.*`, `Makefile`
- **Build system**: `CMakeLists.txt` with automatic Raylib fetch via FetchContent, sanitizer support
- **Network architecture**: Pure epoll-based, non-blocking I/O with `fcntl O_NONBLOCK`, `EPOLLIN|EPOLLOUT|EPOLLET|EPOLLRDHUP`, per-client `RecvBuffer`/`SendBuffer`, length-prefixed TCP framing
- **Raylib GUI** (`src/ui/`):
  - `colors.h` - Official Uno color palette constants
  - `card_renderer.h/cpp` - Procedural rounded-rectangle cards with centered symbols, hover detection
  - `menu_view.h/cpp` - Full menu system: main menu, difficulty selector, local/mixed/LAN setup screens with text input, +/- buttons, Vietnamese rules toggle
  - `game_view.h/cpp` - Game table: green felt background, opponent card-count badges, discard/draw piles with direction indicator, fanned hand with hover lift (20px), click-to-play, color picker overlay for wild cards, game-over screen
  - `gui.h/cpp` - Orchestrator: 60fps loop, mode dispatch, bot turn delay, integration with NetworkServer/NetworkClient
- **MVC Architecture**: Model (`src/core/`, `src/ai/`), View (`src/ui/`), Controller (`src/network/`)
- **Modern C++**: All raw pointers eliminated, `std::unique_ptr` for bot strategies, `std::vector` throughout, no `new`/`delete`
- **Build**: CMake with strict flags, AddressSanitizer + UndefinedBehaviorSanitizer option, `O2` optimization

## Next Steps
1. Build on Linux/WSL: `mkdir build && cd build && cmake .. && make`
2. Install Raylib dev libraries or let CMake fetch them automatically
3. Run `./test_all` to verify all 80 tests
4. Run `./gameuno` to launch the GUI
5. For LAN mode, run server instance and client instances on same/different machines
