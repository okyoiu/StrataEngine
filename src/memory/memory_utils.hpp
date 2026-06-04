#pragma once
#include "core/types.hpp"
#include "core/assert.hpp"
#include <bit>

namespace tessera {
    // cchecks if a number is a power of two
    constexpr bool is_power_of_two(usize n) {
        // gets the binary expression of `n` and uses bitwise operator `&` 
        return (n != 0 && ((n & (n - 1))) == 0);
    }

    // aligns an integer address up to the nearest multiple of `alignment`
    constexpr usize align_up(usize value, usize alignment) {
        TESSERA_ASSERT(is_power_of_two(alignment), "Alignment must be a power of two");
        // `value + alignment - 1` makes sure that if `value` is aligned, it won't be rounded up
        // adding `alignment - 1` makes sure that any remainder when dividing by `alignment` is pushed up
        return (value + alignment - 1) & ~(alignment - 1); // ~(alignment - 1) inverts the bits, creating a mask
    }
}