# Changelog

## [1.0.0] - 2026-07-28

### Added
- State machine pattern (MenuState, PlayingState, GameOverState)
- Animation framework with 7 easing functions
- Particle system for visual effects
- Debug overlay (F3 toggle)
- Audio manager with volume control and mute
- Centralized input manager
- Ref-counted asset manager
- Result<T> error handling utility
- 6-level logging system (console + file)
- Persistent settings system
- Professional documentation (ARCHITECTURE.md, BUILD.md, CONTRIBUTING.md)
- MIT License

### Changed
- Split game_view.cpp into 6 focused render files
- Modern C++ cleanup (enum underlying types, noexcept, final)
- Replaced Gui::run() if-else chain with AppStateMachine
- Simplified gui.h (removed 7 dead methods)
- Removed stale .o build artifacts

### Fixed
- Audio wired to all game events (play, draw, skip, reverse, UNO, win/lose)
- Debug overlay F3 toggle in main loop
- All 80 unit tests pass

## [0.9.0] - 2026-07-27

### Added
- UNO button + catch UNO mechanism
- MT19937 RNG singleton replacing rand()
- Packet validation in network server
- Network server epoll timeout 50ms
- Strict `-Wall -Werror -Wextra -Wshadow` compilation

### Changed
- Replaced all rand() calls with randomInt()
- Removed `using namespace std` from test_all.cpp

## [0.8.0] - 2026-07-26

### Added
- Raylib 2D GUI replacing console UI
- CMake build system with FetchContent
- MVC architecture (core/ai = model, network = controller, ui = view)
- Pure epoll network layer (Linux only)
- Menu system with difficulty/Bot/LAN setup screens
- Card rendering with procedural rounded rectangles
- Direction indicator, force-draw overlay, color picker

### Removed
- console_ui.*, net_platform.*, Makefile
- All Windows compatibility code

## [0.7.0] - 2026-07-25

### Added
- Platform-abstracted network layer (EventLoop interface)
- epoll (Linux) and select (Windows) backends
- TcpReader/TcpWriter with TCP fragmentation handling

## [0.6.0] - 2026-07-24

### Added
- Bot strategies: Easy, Medium, Hard
- Bot factory pattern
- executeBotTurn() in GameEngine

## [0.5.0] - 2026-07-23

### Added
- LAN multiplayer with epoll-based server/client
- Packet types (PLAY_CARD, DRAW, JUMP_IN, CALL_UNO, CATCH_UNO, SYNC_STATE)
- TCP framing with length-prefixed packets
- SyncState serialization

## [0.4.0] - 2026-07-22

### Added
- Vietnamese rules (jump-in, UNO catch, legal last card)
- Color picker for wild cards
- Action card effects (skip, reverse, draw stacking)
- Phase system (DEAL, PLAY, DRAW, JUMP_IN, GAME_OVER)

## [0.3.0] - 2026-07-21

### Added
- Game engine with turn management
- Card validation and play logic
- Draw card and reshuffle mechanics
- Game state tracking

## [0.2.0] - 2026-07-20

### Added
- Player class with hand management
- Card matching rules (canPlayCard, canJumpIn)
- Deck creation and shuffling
- Console-based UI

## [0.1.0] - 2026-07-19

### Added
- Initial project structure
- Card data type with color and number
- Basic build system
- Project documentation
