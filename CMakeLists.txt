add_executable(benchmarks 
    src/memory/pool_allocator.cpp 
    src/memory/arena_allocator.cpp 
    benchmarks/bench_allocators.cpp
)

# Ensure the compiler can find your headers starting from the "src" directory
target_include_directories(benchmarks PRIVATE src)

# Optional: Force extreme optimization for the benchmark target
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(benchmarks PRIVATE -O3 -ffast-math)
endif()