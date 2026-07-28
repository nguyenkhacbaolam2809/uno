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
- All 80 unit tests pass

## Phase 5 - Platform-Abstracted Network Layer ✅
- EventLoop abstraction with epoll/select backends
- TCP fragmentation fixed with two-stage read state machine
- Strict `-Wall -Werror -Wextra -Wshadow` compilation

## Phase 6 - 2D GUI & Modern Architecture ✅
- Raylib GUI replacing console UI; pure epoll network layer
- CMake build system with FetchContent; MVC architecture
- Removed `net_platform.*`, `console_ui.*`, `Makefile`

## Tier 1 - Randomness & Hardening ✅
- MT19937 singleton + `randomInt()` utility in `rng.h`
- All `rand()` calls replaced; `srand()` removed from `main.cpp`
- Network server: 50ms epoll timeout, strict packet validation

## Tier 2 - UNO Button + Catch Mechanism ✅
- Red/gold "UNO!" button, catch UNO targeting, hand scroll offset

## Phase 5 - Production Polish & Architecture Refactor ✅

### 1. UI/UX Foundation (src/utils/)
- **`result.h`** — `Result<T>` template with `isOk()/isFail()` for error-aware resource loading
- **`logger.h/cpp`** — 6-level logger (TRACE..FATAL), console + file output, mutex-guarded, `LOG_INFO()` macros
- **`input_manager.h/cpp`** — Centralized mouse/keyboard/drag/click/double-click/long-press with callback system
- **`asset_manager.h/cpp`** — Ref-counted texture/font/sound/music cache with text texture caching, `loadTexture()` returns `Result<Texture2D>`
- **`animation_manager.h/cpp`** — Full animation framework: `FloatAnim`, `Vec2Anim`, `ColorAnim`, `ShakeAnim`, `DelayAnim`, `SequenceAnim`, `ParallelAnim`. 7 easing functions (linear, quad in/out, back, elastic, bounce). Delta-time based, no per-frame allocs.
- **`particle_system.h/cpp`** — Lightweight particle system with `burst()`, `emit()`, color/velocity/lifetime/size
- **`debug_overlay.h/cpp`** — F3-toggle overlay: FPS, frame time, ping, FPS history graph, key-value info panel
- **`audio_manager.h/cpp`** — SoundId enum, master/music/effects volume sliders, mute toggle, lazy sound loading

### 2. Rendering Refactor (src/ui/)
- **`game_view.cpp`** — Reduced to orchestrator: `render()` calls sub-renderers + particle/animation updates
- **`game_render_board.cpp`** — `renderBackground()`, `renderPiles()` with direction indicator
- **`game_render_cards.cpp`** — `renderHand()`, `getHandCardPos()`, `getCardRect()`, `cardAtPos()`
- **`game_render_players.cpp`** — `renderOpponents()` with card-count badges, `renderTurnIndicator()`
- **`game_render_effects.cpp`** — `renderWinConfetti()` (particle burst), `renderCardGlow()` (sin-based glow)
- **`game_render_ui.cpp`** — `renderUnoButton()`, `renderCatchTargets()`, `renderColorPicker()`, `renderMessageOverlay()`, `handleHandClick()`, `handleUnoCatchClick()`, `showGameOver()`

### 3. Game State Pattern (src/engine/)
- **`game_state.h/cpp`** — `IAppState` interface with `update()/render()/enter()/exit()`
- `MenuState`, `PlayingState`, `GameOverState` implemented
- `AppStateMachine` orchestrates transitions; replaces giant `Gui::run()` if-else chain

### 4. Modern C++ Cleanup
- `card.h`: `enum COLOR : unsigned char` (fixed underlying type), added `noexcept` to `deck` accessors
- `player.h`: `enum PlayerType : unsigned char`, `enum BotDifficulty : unsigned char`
- `deck.h`: `final` class, `noexcept` on `quick_shuffle()`/`add_card()`/`get_size()`
- All allocations use `std::vector` + `reserve()`; no raw `new`/`delete`

### New Files (18 added)
```
src/
  utils/
    result.h              # Result<T> error handling
    logger.h/cpp          # Logging system
    input_manager.h/cpp   # Centralized input
    asset_manager.h/cpp   # Asset cache
    animation_manager.h/cpp  # Animation framework
    particle_system.h/cpp    # Particle effects
    debug_overlay.h/cpp      # F3 debug overlay
    audio_manager.h/cpp      # Audio system
  ui/
    game_render_board.cpp    # Board backgrounds, piles
    game_render_cards.cpp    # Hand rendering
    game_render_players.cpp  # Opponent slots, turn indicator
    game_render_ui.cpp       # UI overlays, color picker, UNO/catch
    game_render_effects.cpp  # Particles, glow, confetti
  engine/
    game_state.h/cpp         # State machine pattern
```

### Modified Files
- `CMakeLists.txt` — Added `utils` library, all new .cpp files, new include directories
- `gui.cpp` — Simplified to use `AppStateMachine` instead of inline game loop
- `game_view.h` — Removed dead members (`showUnoButton`, `catchTarget`, `renderGameOver`), added `cardAtPos()` const+engine param
- `game_view.cpp` — Rewritten as thin orchestrator with particle/animation integration

## Remaining Technical Debt
1. **Audio assets** — Sound files need to be placed in `assets/sounds/` (currently paths are referenced but files may not exist)
2. **Network state pattern** — `PlayingState` only handles local games; LAN server/client need their own state implementations
3. **Text texture caching** — `AssetManager::getCachedText()` is implemented but callers still use `DrawText()` directly
4. **Test suite** — 80 existing tests should still pass; no new tests added yet for new modules
5. **Win/Lose** — `AudioManager::playSound(WIN/LOSE)` not wired into game events yet
6. **`enum COLOR`** — Still unscoped to minimize diff; full migration would touch 20+ files
7. **debug_overlay** — Not yet wired into main loop; needs `IsKeyPressed(KEY_F3)` check in `Gui::run()`

## Build & Run (Linux/WSL)
```bash
mkdir build && cd build && cmake .. && make
./test_all   # verify all 80 tests
./gameuno    # launch GUI
```
