cmake_minimum_required(VERSION 3.16)
project(DeploymentRegistry
    VERSION 0.1.0
    DESCRIPTION "VCP Deployment Registry — tracks deployment records, images, ports and audit history"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(DR_BUILD_TESTS "Build test suite" ON)

# ── Dependencies ──────────────────────────────────────────────────────────────

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system program_options)
find_package(Wt QUIET)

# nlohmann-json for JSON handling
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(NLOHMANN_JSON nlohmann_json)
    endif()
endif()

# If neither find_package nor pkg-config found nlohmann-json, fetch it
if(NOT nlohmann_json_FOUND AND NOT NLOHMANN_JSON_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
    set(nlohmann_json_FOUND TRUE)
endif()

# ── Source subdirectory ───────────────────────────────────────────────────────

add_subdirectory(src)

# ── Copy resources into build directory ───────────────────────────────────────

add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "${CMAKE_BINARY_DIR}/resources"
    COMMENT "Syncing resources → build/resources"
)

add_custom_target(copy_scripts ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/scripts/start.sh" "${CMAKE_BINARY_DIR}/start.sh"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/scripts/env.sh"   "${CMAKE_BINARY_DIR}/env.sh"
    COMMAND chmod +x
        "${CMAKE_BINARY_DIR}/start.sh"
        "${CMAKE_BINARY_DIR}/env.sh"
    COMMENT "Copying scripts/ → build/"
)

# ── Tests ─────────────────────────────────────────────────────────────────────

if(DR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
