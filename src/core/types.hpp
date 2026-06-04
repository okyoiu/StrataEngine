#pragma once
#include <cstdint>
#include <cstddef>

/*  
    ========================================
    DEFINES SOME FIXED-WIDTH INTEGER TYPES
    ALSO INTRODUCING `EntityID` FOR IDENTIFYING
    ENTITIES IN THE ECS
    ========================================
*/

namespace tessera {

    // using fixed-wisth int types
    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using i32 = std::int32_t;
    using i64 = std::int64_t;

    // floating-point types
    using f32 = float;
    using f64 = double;

    // size types
    using usize = std::size_t;
    using isize = std::ptrdiff_t;

    // string typedef for EntityID (Wrapping u32)
    enum class EntityID : u32 {};
}