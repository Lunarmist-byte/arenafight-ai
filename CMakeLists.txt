cmake_minimum_required(VERSION 3.20)
project(ArenaFightAgent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compiler warnings & UTF-8
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find Threads (mandatory for std::mutex and std::thread cross-platform, especially on Linux/POSIX)
find_package(Threads REQUIRED)

# Find Curl
find_package(CURL REQUIRED)

# Find or Fetch nlohmann_json (ensures zero 'works on my machine' issues across all environments)
find_package(nlohmann_json QUIET)
if(NOT TARGET nlohmann_json::nlohmann_json)
    include(FetchContent)
    message(STATUS "nlohmann_json not found locally; fetching via FetchContent...")
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Include paths
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Core sources
set(CORE_SOURCES
    src/common/config.cpp
    src/common/logger.cpp
    src/common/process.cpp
    src/common/http_client.cpp
    src/providers/provider.cpp
    src/providers/ollama_provider.cpp
    src/providers/openrouter_provider.cpp
    src/providers/claude_code_provider.cpp
    src/providers/test_mock_provider.cpp
    src/tools/tool.cpp
    src/tools/file_tools.cpp
    src/tools/search_tools.cpp
    src/tools/shell_tools.cpp
    src/tools/git_tools.cpp
    src/tools/user_tools.cpp
    src/session/session.cpp
    src/session/memory.cpp
    src/session/context_manager.cpp
    src/router/model_router.cpp
    src/planner/planner.cpp
    src/executor/executor.cpp
    src/verifier/verifier.cpp
    src/ui/terminal_ui.cpp
    src/agent/arena_agent.cpp
)

# Core library
add_library(arenafight_core STATIC ${CORE_SOURCES})
target_include_directories(arenafight_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link CURL
if(TARGET CURL::libcurl)
    target_link_libraries(arenafight_core PUBLIC CURL::libcurl)
else()
    target_link_libraries(arenafight_core PUBLIC ${CURL_LIBRARIES})
    target_include_directories(arenafight_core PUBLIC ${CURL_INCLUDE_DIRS})
endif()

# Link Threads
target_link_libraries(arenafight_core PUBLIC Threads::Threads)

# Link nlohmann_json if found as target
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(arenafight_core PUBLIC nlohmann_json::nlohmann_json)
endif()

if(WIN32)
    target_link_libraries(arenafight_core PUBLIC ws2_32 crypt32)
endif()

# Main executable: arena
add_executable(arena src/main.cpp)
target_link_libraries(arena PRIVATE arenafight_core)

# Test executable: arena_test
add_executable(arena_test tests/test_suite.cpp)
target_link_libraries(arena_test PRIVATE arenafight_core)

enable_testing()
add_test(NAME ArenaFightTests COMMAND arena_test)
