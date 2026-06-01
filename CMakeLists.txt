cmake_minimum_required(VERSION 3.21)
project(avail
    VERSION 0.1.0
    DESCRIPTION "avail - prototype rocprof-sys-avail UX"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AVAIL_BUILD_TESTS "Build avail tests" ON)
option(AVAIL_COVERAGE "Enable coverage instrumentation" OFF)

include(FetchContent)

# profiling-catalogue (INTERFACE library, single source of truth for metric descriptors).
FetchContent_Declare(
    profcat
    SOURCE_DIR $ENV{HOME}/work/profiling-catalogue
)
FetchContent_MakeAvailable(profcat)

# Core library.
add_library(avail_core STATIC
    src/render.cpp
    src/screens.cpp
    src/session.cpp
    src/synthesize.cpp
    src/app.cpp
    src/input.cpp
    src/menu.cpp
    src/mocks_online.cpp
    src/mocks_offline.cpp
    src/version.cpp
)

target_include_directories(avail_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(avail_core PUBLIC profcat::profcat)

target_compile_options(avail_core PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)

target_compile_definitions(avail_core PUBLIC
    AVAIL_VERSION_STRING="${PROJECT_VERSION}"
)

# Main executable.
add_executable(avail src/main.cpp)
target_link_libraries(avail PRIVATE avail_core)
target_compile_options(avail PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)

if(AVAIL_COVERAGE)
    target_compile_options(avail_core PUBLIC --coverage -O0 -g)
    target_link_options(avail_core PUBLIC --coverage)
endif()

if(AVAIL_BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()
