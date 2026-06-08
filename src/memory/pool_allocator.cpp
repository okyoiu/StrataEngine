#include "memory/pool_allocator.hpp"
#include "memory/memory_utils.hpp"
#include "core/assert.hpp"

namespace tessera {

    PoolAllocator::PoolAllocator(void* backing_buffer, usize buffer_size, usize block_size)
                    : backing_buffer_(backing_buffer),
                    buffer_size_(buffer_size),
                    block_size_(block_size),
                    block_count_(buffer_size / block_size),
                    free_list_head_(nullptr) {
                        TESSERA_ASSERT(backing_buffer_ != nullptr, "Backing buffer cannot be null");
                        TESSERA_ASSERT(block_size_ >= sizeof(void*), "Block size must be >= 8 bytes to hold a pointer");
                        TESSERA_ASSERT(buffer_size_ >= block_size_, "Buffer size is too small for even one block");
                        initialize_free_list();
                    }


    void PoolAllocator::initialize_free_list() {
        free_list_head_ = backing_buffer_;

        // thread the free list thru the blocks
        for (usize i =0; i < block_count_ - 1; ++i) {
            void* current_block = ptr_add(backing_buffer_, i * block_size_);
            void* next_block = ptr_add(backing_buffer_, (i + 1) * block_size_);

            // write address of next block into the first 8 bytes of current block
            *reinterpret_cast<void**>(current_block) = next_block;
        }
        // terminate the tail of the list
        void* last_block = ptr_add(backing_buffer_, (block_count_ - 1) * block_size_);
        *reinterpret_cast<void**>(last_block) = nullptr;
    }

    void* PoolAllocator::allocate(usize bytes, usize alignment) {
        TESSERA_ASSERT(bytes <= block_size_, "Requested bytes exceed fixed block size");
        TESSERA_ASSERT(reinterpret_cast<usize>(free_list_head_) % alignment == 0, "Free list head fails alignment");
        TESSERA_ASSERT(free_list_head_ != nullptr, "FATAL: Pool allocator exhausted");
    
        // Pop the head of the free list
        void* allocated_block = free_list_head_;
        
        // The new head becomes whatever the current head was pointing to
        free_list_head_ = *reinterpret_cast<void**>(allocated_block);
    
        return allocated_block;
    }
    
    void PoolAllocator::deallocate(void* ptr, usize /*bytes*/) {
        if (ptr == nullptr) return;
    
        // Bounds checking to catch rogue pointers during development
        TESSERA_ASSERT(ptr >= backing_buffer_ && ptr < ptr_add(backing_buffer_, buffer_size_), 
                       "Deallocated pointer is out of bounds of this pool");
    
        // Push the freed block to the front of the free list
        *reinterpret_cast<void**>(ptr) = free_list_head_;
        free_list_head_ = ptr;
    }
    
    void PoolAllocator::reset() {
        // Re-threading the list restores all memory to a free state
        initialize_free_list();
    }
}