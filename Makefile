COMPILER = g++
LINKER = g++
INCLUDE = -Isrc/core -Isrc/ai -Isrc/network -Isrc/ui

CORE = src/core
AI = src/ai
NET = src/network
UI = src/ui

CORE_OBJ = card.o deck.o player.o config.o rules.o game_engine.o
AI_OBJ = bot_easy.o bot_medium.o bot_hard.o bot_factory.o
NET_OBJ = network_server.o network_client.o net_platform_shared.o
UI_OBJ = console_ui.o

ifeq ($(OS),Windows_NT)
    PLATFORM_OBJ = net_platform_win.o
    LIBS = -lws2_32
    COMPILER_FLAGS = -c -g -O0 -Wall -Werror -Wextra -Wshadow -std=c++17
    LFLAGS = -static-libgcc -static-libstdc++
else
    PLATFORM_OBJ = net_platform_epoll.o
    LIBS =
    COMPILER_FLAGS = -c -g -O0 -Wall -Werror -Wextra -Wshadow -std=c++17
    LFLAGS =
endif

OBJS = main.o $(CORE_OBJ) $(AI_OBJ) $(NET_OBJ) $(PLATFORM_OBJ) $(UI_OBJ)

gameuno: $(OBJS)
	$(LINKER) $(OBJS) -o gameuno $(LIBS) $(LFLAGS)

main.o: main.cpp $(CORE)/config.h $(UI)/console_ui.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) main.cpp

card.o: $(CORE)/card.cpp $(CORE)/card.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(CORE)/card.cpp

deck.o: $(CORE)/deck.cpp $(CORE)/deck.h $(CORE)/card.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(CORE)/deck.cpp

player.o: $(CORE)/player.cpp $(CORE)/player.h $(CORE)/card.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(CORE)/player.cpp

config.o: $(CORE)/config.cpp $(CORE)/config.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(CORE)/config.cpp

rules.o: $(CORE)/rules.cpp $(CORE)/rules.h $(CORE)/card.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(CORE)/rules.cpp

game_engine.o: $(CORE)/game_engine.cpp $(CORE)/game_engine.h $(CORE)/rules.h $(CORE)/deck.h $(CORE)/player.h $(AI)/bot_factory.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(CORE)/game_engine.cpp

bot_easy.o: $(AI)/bot_easy.cpp $(AI)/bot_easy.h $(AI)/ibot_strategy.h $(CORE)/rules.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(AI)/bot_easy.cpp

bot_medium.o: $(AI)/bot_medium.cpp $(AI)/bot_medium.h $(AI)/ibot_strategy.h $(CORE)/rules.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(AI)/bot_medium.cpp

bot_hard.o: $(AI)/bot_hard.cpp $(AI)/bot_hard.h $(AI)/ibot_strategy.h $(CORE)/rules.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(AI)/bot_hard.cpp

bot_factory.o: $(AI)/bot_factory.cpp $(AI)/bot_factory.h $(AI)/bot_easy.h $(AI)/bot_medium.h $(AI)/bot_hard.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(AI)/bot_factory.cpp

network_server.o: $(NET)/network_server.cpp $(NET)/network_server.h $(NET)/packets.h $(NET)/net_platform.h $(CORE)/game_engine.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(NET)/network_server.cpp

network_client.o: $(NET)/network_client.cpp $(NET)/network_client.h $(NET)/packets.h $(NET)/net_platform.h $(CORE)/game_engine.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(NET)/network_client.cpp

net_platform_shared.o: $(NET)/net_platform_shared.cpp $(NET)/net_platform.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(NET)/net_platform_shared.cpp

net_platform_win.o: $(NET)/net_platform_win.cpp $(NET)/net_platform.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(NET)/net_platform_win.cpp

net_platform_epoll.o: $(NET)/net_platform_epoll.cpp $(NET)/net_platform.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(NET)/net_platform_epoll.cpp

console_ui.o: $(UI)/console_ui.cpp $(UI)/console_ui.h $(CORE)/config.h $(CORE)/game_engine.h $(NET)/network_server.h $(NET)/network_client.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) $(UI)/console_ui.cpp

test_all: test_all.o $(CORE_OBJ) $(AI_OBJ)
	$(LINKER) test_all.o $(CORE_OBJ) $(AI_OBJ) -o test_all $(LIBS) $(LFLAGS)

test_all.o: tests/test_all.cpp $(CORE)/card.h $(CORE)/deck.h $(CORE)/player.h $(CORE)/rules.h $(CORE)/game_engine.h $(CORE)/config.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE) tests/test_all.cpp

clean:
	-del /Q *.o 2>NUL
ifneq ($(OS),Windows_NT)
	-rm -f *.o
endif

distclean: clean
	-del /Q gameuno.exe 2>NUL
ifneq ($(OS),Windows_NT)
	-rm -f gameuno
endif
