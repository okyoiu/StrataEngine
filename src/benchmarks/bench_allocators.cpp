#include "memory/pool_allocator.hpp"
#include "memory/arena_allocator.hpp"
#include "core/assert.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>

namespace tessera {
    namespace bench {
        // Benchmarking Stuff/Utils (for benchmarking)

        // preventing compiler from optimizing away allocations (doesnt remove stuff to make results accurate)
        template <typename T>
        inline void DoNotOptimize(T const& value) {
            #if defined(__clang__) || defined(__GNUC__) // compiler check
                // if value is read or written, compiler cannot optimize it, memory might be modified
                asm volatile("": : "r,m"(value) : "memory"); // either r-register or memory
            #else
                volatile T sink = value;
                (void)sink;
            #endif
        }

        // gets high-resolution timestamp in nanoseconds
        inline double get_time_ns() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<double, std::nano>(now.time_since_epoch()).count();
        }

        void assert_speedup(double baseline_ns, double candidate_ns, double min_expected_speedup, const char* test_name) {
            double speedup = baseline_ns / candidate_ns;
            std::cout << std::left << std::setw(35) << test_name 
              << " | Base: " << std::setw(8) << std::fixed << std::setprecision(1) << baseline_ns << " ns"
              << " | Custom: " << std::setw(8) << candidate_ns << " ns"
              << " | Speedup: " << std::setw(5) << speedup << "x";

              if (speedup >= min_expected_speedup) {
                std::cout << " [PASS]\n";
              } else {
                std::cout << " [FAIL] (Expected " << min_expected_speedup << "x)\n";
              }
        }


        // THE BENCHMARKS
        void run_pool_benchmark() {
            constexpr usize ITERATIONS = 100'000;
            constexpr usize BLOCK_SIZE = 64; // Size of a typical ECS Component
            constexpr usize ALIGNMENT = 8;

            std::vector<void*> ptrs(ITERATIONS, nullptr);

            // allocating a 6.4MB slab for the pool
            std::vector<u8> pool_backing(ITERATIONS * BLOCK_SIZE);
            PoolAllocator pool(pool_backing.data(), pool_backing.size(), BLOCK_SIZE);

            // warmup
            for (usize i = 0; i < 1000; ++i) {
                void* p = pool.allocate(BLOCK_SIZE, ALIGNMENT);
                DoNotOptimize(p);
                pool.deallocate(p, BLOCK_SIZE);
            }

            // 1. Standard new/delete Allocation
            double start = get_time_ns();
            for (usize i = 0; i < ITERATIONS; ++i) {
                ptrs[i] = ::operator new(BLOCK_SIZE);
                DoNotOptimize(ptrs[i]);
            }
            double baseline_alloc = (get_time_ns() - start) / ITERATIONS;

            // Standard new/delete Deallocation
            start = get_time_ns();
            for (usize i = 0; i < ITERATIONS; ++i) {
                ::operator delete(ptrs[i]);
            }
            double baseline_dealloc = (get_time_ns() - start) / ITERATIONS;

            // 2. Pool Allocation
            start = get_time_ns();
            for (usize i = 0; i < ITERATIONS; ++i) {
                ptrs[i] = pool.allocate(BLOCK_SIZE, ALIGNMENT);
                DoNotOptimize(ptrs[i]);
            }
            double pool_alloc = (get_time_ns() - start) / ITERATIONS;

            // Pool Deallocation
            start = get_time_ns();
            for (usize i = 0; i < ITERATIONS; ++i) {
                pool.deallocate(ptrs[i], BLOCK_SIZE);
            }
            double pool_dealloc = (get_time_ns() - start) / ITERATIONS;

            std::cout << "--- PoolAllocator vs Native Heap (" << ITERATIONS << " ops) ---\n";
            assert_speedup(baseline_alloc, pool_alloc, 3.0, "Allocation (64b block)");
            assert_speedup(baseline_dealloc, pool_dealloc, 5.0, "Deallocation (64b block)");
            std::cout << "\n";
        }

        void run_arena_benchmark() {
            constexpr usize ITERATIONS = 10'000;
            constexpr usize ALLOC_SIZE = 128; 
            constexpr usize ALIGNMENT = 16;
        
            std::vector<void*> ptrs(ITERATIONS, nullptr);
            std::vector<u8> arena_backing(ITERATIONS * (ALLOC_SIZE + ALIGNMENT));
            ArenaAllocator arena(arena_backing.data(), arena_backing.size());
        
            // 1. Standard Allocation (to setup the delete baseline)
            for (usize i = 0; i < ITERATIONS; ++i) {
                ptrs[i] = ::operator new(ALLOC_SIZE);
            }
        
            // Baseline: N individual deletes
            double start = get_time_ns();
            for (usize i = 0; i < ITERATIONS; ++i) {
                ::operator delete(ptrs[i]);
            }
            double baseline_dealloc = get_time_ns() - start;
        
            // 2. Arena Allocation
            for (usize i = 0; i < ITERATIONS; ++i) {
                ptrs[i] = arena.allocate(ALLOC_SIZE, ALIGNMENT);
                DoNotOptimize(ptrs[i]);
            }
        
            // Arena: O(1) bulk reset
            start = get_time_ns();
            arena.reset();
            double arena_reset = get_time_ns() - start;
        
            std::cout << "--- ArenaAllocator vs Native Heap (" << ITERATIONS << " ops) ---\n";
            assert_speedup(baseline_dealloc, arena_reset, 50.0, "Bulk Free vs N deletes");
            std::cout << "\n";
        }
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << " Tessera Memory Subsystem Benchmarks\n";
    std::cout << "========================================\n\n";

    tessera::bench::run_pool_benchmark();
    tessera::bench::run_arena_benchmark();

    return 0;
}