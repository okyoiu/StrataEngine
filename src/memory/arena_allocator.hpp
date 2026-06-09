#pragma once
#include "memory/allocator_interface.hpp"
#include "core/types.hpp"

namespace tessera {
    class ArenaAllocator {
        public:
            // Operates on a caller-provided memory block (enables composition)
            ArenaAllocator(void* backing_buffer, usize buffer_size);
            ~ArenaAllocator() = default;

            // Non-copyable, non-movable
            ArenaAllocator(const ArenaAllocator&) = delete;
            ArenaAllocator& operator=(const ArenaAllocator&) = delete;

            // Satisfies IAllocator concept
            void* allocate(usize bytes, usize alignment);
            void deallocate(void* ptr, usize bytes);
            void reset();

            // Diagnostics
            usize get_peak_usage() const { return peak_usage_; }
            usize get_current_usage() const { return reinterpret_cast<usize>(current_) - reinterpret_cast<usize>(base_); }
        private:
            void* base_;
            void* current_;
            void* end_;
            usize peak_usage_;
    };
}