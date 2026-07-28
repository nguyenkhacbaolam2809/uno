cmake_minimum_required(VERSION 3.16)
project(gameuno CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)
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

add_library(network
    src/network/network_server.cpp
    src/network/network_client.cpp
)

target_include_directories(network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(network PUBLIC gamecore)

add_executable(gameuno
    main.cpp
    src/ui/gui.cpp
    src/ui/card_renderer.cpp
    src/ui/game_view.cpp
    src/ui/menu_view.cpp
)

target_include_directories(gameuno PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
)

target_link_libraries(gameuno
    gamecore
    network
    raylib
)

if(ENABLE_SANITIZERS)
    target_compile_options(gamecore PRIVATE -fsanitize=address,undefined)
    target_compile_options(network PRIVATE -fsanitize=address,undefined)
    target_compile_options(gameuno PRIVATE -fsanitize=address,undefined)
    target_link_libraries(gamecore PRIVATE -fsanitize=address,undefined)
    target_link_libraries(network PRIVATE -fsanitize=address,undefined)
    target_link_libraries(gameuno PRIVATE -fsanitize=address,undefined)
endif()

target_compile_options(gamecore PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror)
target_compile_options(network PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror)
target_compile_options(gameuno PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror -O2)

if(BUILD_TESTS)
    add_executable(test_all
        tests/test_all.cpp
    )
    target_include_directories(test_all PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
        ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    )
    target_link_libraries(test_all gamecore)
    target_compile_options(test_all PRIVATE -Wall -Wextra -Wshadow -Wpedantic -Werror -O2)
endif()
