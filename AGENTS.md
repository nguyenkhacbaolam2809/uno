# Project Progress

## Phase 1 - Foundation ✅
- Core data types: `card.h/cpp`, `deck.h/cpp`, `player.h/cpp`
- Game configuration: `config.h/cpp`
- Game logic: `rules.h/cpp`
- UI layer: `console_ui.h/cpp` with Vietnamese/English support
- Makefile build system
- Singleplayer, local multiplayer, mixed (human+bot), LAN modes

## Phase 2 - AI Bot Engine with Strategy Pattern ✅
- `ibot_strategy.h` - Strategy interface with virtual methods
- `bot_easy.h/cpp` - Easy: plays first legal card or draws
- `bot_medium.h/cpp` - Medium: prioritizes action cards, wilds when lacking color
- `bot_hard.h/cpp` - Hard: strategic color counting, card value optimization, wild preference
- `bot_factory.h/cpp` - Factory to create bot strategies by difficulty
- `game_engine.h/cpp` - Updated with `executeBotTurn()`, `createBot()`, `BotActionResult`
- Removed old `bot_strategy.h/cpp`
- Makefile updated for new file structure

## Phase 3 - Network Layer ✅
- `packets.h` - Packet types (PLAY_CARD, DRAW, JUMP_IN, CALL_UNO, CATCH_UNO, SYNC_STATE)
- `network_server.h/cpp` - TCP server with Winsock2:
  - Listen for connections, manage up to 4 clients
  - Authoritative GameEngine per session
  - Client receive threads with thread-safe sync state broadcast
  - Full game state serialization (players, hands, current card, etc.)
- `network_client.h/cpp` - TCP client:
  - Connect to remote server
  - Send actions (play card, draw, jump in, uno, catch uno)
  - Receive and deserialize game state into SyncState struct
- `config.cpp` - LAN menu messages (host/join prompts, IP/port input)
- `console_ui.h/cpp` - Split modeLan into `modeLanServer()` (host) and `modeLanClient()` (join)
- `Makefile` - Added network objects + `-lws2_32` for Winsock2 linking

## Phase 4 - Testing & Polish ✅
- Comprehensive unit tests: 80 tests covering card, deck, player, rules, game engine, and bots
- Fixed `addPlayer` bug: `init()` now creates players with empty names so `addPlayer` can replace them
- Fixed `isLegalLastCard`: skip (11) and reverse (12) are now correctly allowed as last cards
- Fixed `isLegalLastCard`: only draw2 (10), wild (13), wild draw4 (14) are forbidden
- Fixed Skip/Reverse double-advance bug in `applyActionCard`
- Fixed `nextTurn()` loop safety check
- Fixed `chooseRandomStarter()` infinite loop protection
- Fixed Fisher-Yates shuffle off-by-one in `quick_shuffle()`
- Removed `srand()` from shuffle loops (seeded once in `main()`)
- Fixed `reshuffleDiscard()` empty deck edge case
- Fixed `peek()` bounds checking in `player.cpp`
- Fixed `waitForEnter()` not clearing cin fail-bit
- Fixed `pickCardFromHand()` and `pickColor()` infinite loops on non-numeric input
- Fixed force-draw `peek(-1)` crash in `handleHumanTurn()`
- Fixed main menu `cin` failure handling
- Fixed `callUno()` stub implementation
- Fixed `network_server.cpp`: stack buffer overflow, packet body validation, player slot bug, thread cleanup, critical section lifecycle, code deduplication
- Removed old `test_legacy.cpp` (preprocessor macro based, only 1 test path compiled)
- Added `test_all.cpp` with 80 passing tests
- Added `test_all` target to Makefile
