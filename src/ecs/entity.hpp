#pragma once
#include "core/types.hpp"

namespace tessera {
    // allocating 20 bits for the index
    inline constexpr u32 ENTITY_INDEX_BITS = 20;
    inline constexpr u32 ENTITY_INDEX_MASK = (1 << ENTITY_INDEX_BITS) - 1;

    // We allocate 12 bits for the version (4096 generations per index)
    inline constexpr u32 ENTITY_VERSION_BITS = 12;
    inline constexpr u32 ENTITY_VERSION_MASK = (1 << ENTITY_VERSION_BITS) - 1;

    // Sentinel value to represent an invalid or null entity
    inline constexpr EntityID NULL_ENTITY = static_cast<EntityID>(0xFFFFFFFF);

    // Extracts the 20-bit array index from an EntityID
    constexpr u32 entity_index(EntityID entity) {
        return static_cast<u32>(entity) & ENTITY_INDEX_MASK;
    }
    // Extracts the 12-bit generation version from an EntityID
    constexpr u32 entity_version(EntityID entity) {
        return (static_cast<u32>(entity) >> ENTITY_INDEX_BITS) & ENTITY_VERSION_MASK;
    }

    // Packs a raw index and version into a single 32-bit EntityID
    constexpr EntityID make_entity_id(u32 index, u32 version) {
        return static_cast<EntityID>(
            (index & ENTITY_INDEX_MASK) | ((version & ENTITY_VERSION_MASK) << ENTITY_INDEX_BITS)
        );
    }
}