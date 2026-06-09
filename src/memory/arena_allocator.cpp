#include "memory/arena_allocator.hpp"
#include "memory/memory_utils.hpp"
#include "core/platform.hpp" 
#include "core/assert.hpp"   
#include <algorithm>

namespace tessera {
    ArenaAllocator::ArenaAllocator(void* backing_buffer, usize buffer_size)
    : base_(backing_buffer),
      current_(backing_buffer),
      end_(ptr_add(backing_buffer, buffer_size)),
      peak_usage_(0) {
        TESSERA_ASSERT(base_ != nullptr, "Arena backing buffer cannot be null");
        TESSERA_ASSERT(buffer_size > 0, "Arena capacity must be greater than zero");
    }

    void* ArenaAllocator::allocate(usize bytes, usize alignment) {
        if (bytes == 0) {
            TESSERA_ASSERT(bytes != 0, "Allocation size cannot be zero");
            return nullptr;
        }
        
        // Find the nearest aligned address starting from our current position
        void* aligned_ptr = align_ptr(current_, alignment);
        void* next_current = ptr_add(aligned_ptr, bytes);

        // Hard out-of-memory boundary check
        if (next_current > end_) {
            TESSERA_ASSERT(false, "Arena Allocator out of memory! Capacity exceeded.");
            return nullptr;
        }
        
        // Advance the tape
        current_ = next_current;

        // Update diagnostic metrics
        usize current_offset = reinterpret_cast<usize>(current_) - reinterpret_cast<usize>(base_);
        peak_usage_ = std::max(peak_usage_, current_offset);

        return aligned_ptr;
    }

    void ArenaAllocator::deallocate(void* /*ptr*/, usize /*bytes*/) {
        // Intentional design invariant: Individual deallocations are a no-op.
        // Memory is reclaimed collectively via reset().
    }

    void ArenaAllocator::reset() {
        // Rewind the tape pointer back to the origin. O(1) bulk free.
        current_ = base_;
    }
}