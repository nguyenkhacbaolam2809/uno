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

## Phase 4 - Testing & Polish (Pending)
- Unit tests
- Edge case handling
- Performance optimization
- Code cleanup
