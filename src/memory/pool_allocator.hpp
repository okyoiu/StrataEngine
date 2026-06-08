#pragma once
#include "memory/allocator_interface.hpp"
#include"core/types.hpp"

namespace tessera {

class PoolAllocator {
    public:
        // taking ownership of pre-allocated memory
        PoolAllocator(void* backing_buffer, usize buffer_size, usize block_size);
        ~PoolAllocator() = default;

        // strict ownership
        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;
        
        // satisfying IAllocator concept
        void* allocate(usize bytes, usize alignment);
        void deallocate(void* ptr, usize bytes);
        void reset();
    private:
        void initialize_free_list();
    
        void* backing_buffer_;
        usize buffer_size_;
        usize block_size_;
        usize block_count_;
        void* free_list_head_;
    };
}