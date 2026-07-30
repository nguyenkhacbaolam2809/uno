cmake_minimum_required(VERSION 3.16)
project(gameuno CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
else()
    option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)
endif()
option(BUILD_TESTS "Build the test suite" ON)

add_library(gamecore
    src/core/card.cpp
    src/core/deck.cpp
    src/core/player.cpp
    src/core/config.cpp
    src/core/rules.cpp
    src/core/game_engine.cpp
    src/ai/bot_easy.cpp
    src/ai/bot_medium.cpp
    src/ai/bot_hard.cpp
    src/ai/bot_factory.cpp
)

target_include_directories(gamecore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
)

find_package(raylib QUIET)
if(NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
    )
    FetchContent_GetProperties(raylib)
    if(NOT raylib_POPULATED)
        set(FETCHCONTENT_QUIET OFF)
        FetchContent_Populate(raylib)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
    endif()
endif()

if(NOT WIN32)
    add_library(network
        src/network/network_server.cpp
        src/network/network_client.cpp
    )

    target_include_directories(network PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/network
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    )

    target_link_libraries(network PUBLIC gamecore)
endif()

add_library(utils
    src/utils/logger.cpp
    src/utils/input_manager.cpp
    src/utils/asset_manager.cpp
    src/utils/animation_manager.cpp
    src/utils/particle_system.cpp
    src/utils/debug_overlay.cpp
    src/utils/audio_manager.cpp
    src/utils/settings.cpp
)

target_include_directories(utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(utils PUBLIC raylib)

add_executable(gameuno
    main.cpp
    src/ui/gui.cpp
    src/ui/card_renderer.cpp
    src/ui/game_view.cpp
    src/ui/game_render_board.cpp
    src/ui/game_render_cards.cpp
    src/ui/game_render_players.cpp
    src/ui/game_render_ui.cpp
    src/ui/game_render_effects.cpp
    src/ui/menu_view.cpp
    src/ui/console_ui.cpp
    src/engine/game_state.cpp
)

target_include_directories(gameuno PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
)

target_link_libraries(gameuno PRIVATE
    gamecore
    utils
    raylib
)
if(NOT WIN32)
    target_link_libraries(gameuno PRIVATE network)
endif()

if(MSVC)
    target_compile_options(gamecore PRIVATE /W4 /WX /permissive-)
    target_compile_options(utils PRIVATE /W4 /WX /permissive-)
    target_compile_options(gameuno PRIVATE /W4 /WX /permissive- /O2)
else()
    target_compile_options(gamecore PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror)
    target_compile_options(utils PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror)
    target_compile_options(gameuno PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror -O2)
endif()

if(ENABLE_SANITIZERS AND NOT WIN32)
    target_compile_options(gamecore PRIVATE -fsanitize=address,undefined)
    target_compile_options(utils PRIVATE -fsanitize=address,undefined)
    target_compile_options(gameuno PRIVATE -fsanitize=address,undefined)
    target_link_libraries(gamecore PRIVATE -fsanitize=address,undefined)
    target_link_libraries(utils PRIVATE -fsanitize=address,undefined)
    target_link_libraries(gameuno PRIVATE -fsanitize=address,undefined)
    if(NOT WIN32)
        target_compile_options(network PRIVATE -fsanitize=address,undefined)
        target_link_libraries(network PRIVATE -fsanitize=address,undefined)
    endif()
endif()

if(BUILD_TESTS)
    add_executable(test_all
        tests/test_all.cpp
    )
    target_include_directories(test_all PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
        ${CMAKE_CURRENT_SOURCE_DIR}/src/network
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    )
    target_link_libraries(test_all gamecore)
    if(MSVC)
        target_compile_options(test_all PRIVATE /W4 /WX /permissive- /O2)
    else()
        target_compile_options(test_all PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror -O2)
    endif()
endif()
