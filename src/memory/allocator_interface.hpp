#pragma once
#include "core/types.hpp"
#include <concepts>

namespace tessera {
    // checks if type `T` satisfies the requirements of a memory allocator
    template <typename T>
    concept IAllocator = requires(T allocator, usize bytes, usize alignment, void* ptr) { // clause of the requiremnets that type `T` must satisfy
        { allocator.allocate(bytes, alignment) } -> std::same_as<void*>; // makes sure that it returns type of allocate as `void*`
        { allocator.deallocate(ptr, bytes) } -> std::same_as<void>;
        { allocator.reset() } -> std::same_as<void>;
    };
}