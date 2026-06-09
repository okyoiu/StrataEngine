cmake_minimum_required(VERSION 3.20)
project(tessera VERSION 0.1.0 LANGUAGES CXX)

# Require strictly C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disables compiler-specific extensions

# Global warning flags for robust systems programming
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion")

# ------------------------------------------------------------------------------
# 1. Main Simulation Executable
# ------------------------------------------------------------------------------
# We bundle the memory subsystem files you've created so far.
add_executable(tessera_engine 
    src/main.cpp
    src/memory/pool_allocator.cpp
    src/memory/arena_allocator.cpp
)

# Tell the compiler where to resolve includes (e.g., #include "core/types.hpp")
target_include_directories(tessera_engine PRIVATE src)

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

# ------------------------------------------------------------------------------
# 3. Future SDL2 Integration (Milestone 5)
# ------------------------------------------------------------------------------
# We will uncomment and use these lines when we build the hardware-accelerated frontend.
# find_package(SDL2 REQUIRED)
# target_link_libraries(tessera_engine PRIVATE SDL2::SDL2)