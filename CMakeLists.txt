cmake_minimum_required(VERSION 3.20)
project(tessera VERSION 0.1.0 LANGUAGES CXX)

# Require strictly C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disables compiler-specific extensions

# Global warning flags for robust systems programming
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion")

# Homebrew installs its CMake configs under a versioned Cellar path that isn't
# always on the default search path — this covers both Apple Silicon and Intel.
list(APPEND CMAKE_PREFIX_PATH /opt/homebrew /usr/local)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# ------------------------------------------------------------------------------
# 1. Main Simulation Executable
# ------------------------------------------------------------------------------
add_executable(tessera_engine
    src/main.cpp
    src/memory/pool_allocator.cpp
    src/memory/arena_allocator.cpp
    src/spatial/spatial_grid.cpp
    src/render/renderer.cpp
    src/app/input_handler.cpp
    src/app/app.cpp
)

# Two include roots: the project root resolves the "src/..." style used by
# ecs/spatial/simulation/game/render/app headers, while "src" itself resolves
# the older bare "core/..." / "memory/..." style used by the memory subsystem.
target_include_directories(tessera_engine PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)

target_include_directories(tessera_engine PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(tessera_engine PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf)

# ------------------------------------------------------------------------------
# 2. Memory Benchmarks Executable
# ------------------------------------------------------------------------------
add_executable(benchmarks 
    benchmarks/bench_allocators.cpp
    src/memory/pool_allocator.cpp
    src/memory/arena_allocator.cpp
)

target_include_directories(benchmarks PRIVATE src)

# Force extreme compiler optimization strictly for the benchmark target
# This ensures we measure hardware performance, not unoptimized debug bloat.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(benchmarks PRIVATE -O3 -ffast-math -DNDEBUG)
endif()