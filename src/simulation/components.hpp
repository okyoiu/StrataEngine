#pragma once
#include "src/core/types.hpp"

#include <type_traits>

namespace tessera::sim {

// -----------------------------------------------------------------------------
// Tunables
// -----------------------------------------------------------------------------
inline constexpr u32 MINER_TICKS_PER_ITEM   = 30; // ~0.5s at 60 ticks/sec
inline constexpr u32 MINER_MAX_BUFFER       = 5;
inline constexpr u32 BELT_TICKS_PER_MOVE    = 15;
inline constexpr u32 INSERTER_TICKS_PER_MOVE = 20;

// -----------------------------------------------------------------------------
// ResourceType — the tradeable goods that flow through the factory
// -----------------------------------------------------------------------------
enum class ResourceType : u8 { Iron, Copper, Coal, Stone, Gold, Count };

inline constexpr usize RESOURCE_TYPE_COUNT = static_cast<usize>(ResourceType::Count);

[[nodiscard]] constexpr const char* resource_name(ResourceType type) noexcept {
    switch (type) {
        case ResourceType::Iron:   return "Iron";
        case ResourceType::Copper: return "Copper";
        case ResourceType::Coal:   return "Coal";
        case ResourceType::Stone:  return "Stone";
        case ResourceType::Gold:   return "Gold";
        default:                   return "?";
    }
}

[[nodiscard]] constexpr f64 resource_price(ResourceType type) noexcept {
    switch (type) {
        case ResourceType::Iron:   return 2.0;
        case ResourceType::Copper: return 3.0;
        case ResourceType::Coal:   return 1.5;
        case ResourceType::Stone:  return 1.0;
        case ResourceType::Gold:   return 8.0;
        default:                   return 0.0;
    }
}

// -----------------------------------------------------------------------------
// Direction — one of 4 cardinal facings used by belts, inserters, and miners
// -----------------------------------------------------------------------------
enum class Direction : u8 { North = 0, East = 1, South = 2, West = 3 };

[[nodiscard]] constexpr Direction rotate_cw(Direction d) noexcept {
    return static_cast<Direction>((static_cast<u8>(d) + 1) % 4);
}

[[nodiscard]] constexpr Direction opposite(Direction d) noexcept {
    return static_cast<Direction>((static_cast<u8>(d) + 2) % 4);
}

struct GridPosition {
    i32 x = 0;
    i32 y = 0;
};

[[nodiscard]] constexpr GridPosition direction_delta(Direction d) noexcept {
    switch (d) {
        case Direction::North: return {0, -1};
        case Direction::East:  return {1, 0};
        case Direction::South: return {0, 1};
        case Direction::West:  return {-1, 0};
        default:               return {0, 0};
    }
}

[[nodiscard]] constexpr GridPosition operator+(GridPosition a, GridPosition b) noexcept {
    return {a.x + b.x, a.y + b.y};
}

// -----------------------------------------------------------------------------
// Buildings — every placed building carries GridPosition + Building + exactly
// one of the kind-specific components below.
// -----------------------------------------------------------------------------
enum class BuildingKind : u8 { None, Miner, Belt, Inserter, Seller };

struct Building {
    BuildingKind kind = BuildingKind::None;
};

// Natural terrain feature. Never created/destroyed by the player; a Miner must
// be placed directly on top of one to extract `type`.
struct ResourceNode {
    ResourceType type;
};

// Extracts `type` from the ResourceNode beneath it into an internal buffer,
// then pushes finished units onto whatever sits in `facing`.
struct Miner {
    Direction facing;
    ResourceType type;
    u32 progress = 0;
    u32 buffer = 0;
};

// Conveyor segment. Holds at most one item at a time and pushes it forward
// once `progress` reaches BELT_TICKS_PER_MOVE.
struct BeltLane {
    Direction facing;
    bool has_item = false;
    ResourceType item_type = ResourceType::Iron;
    u32 progress = 0;
};

// Picks an item off the tile behind it and drops it on the tile ahead of it
// every INSERTER_TICKS_PER_MOVE ticks.
struct Inserter {
    Direction facing;
    u32 progress = 0;
};

// Converts any item deposited on it directly into money.
struct Seller {
    u64 accepted_count = 0;
};

static_assert(std::is_trivially_copyable_v<Building>);
static_assert(std::is_trivially_copyable_v<ResourceNode>);
static_assert(std::is_trivially_copyable_v<Miner>);
static_assert(std::is_trivially_copyable_v<BeltLane>);
static_assert(std::is_trivially_copyable_v<Inserter>);
static_assert(std::is_trivially_copyable_v<Seller>);
static_assert(std::is_trivially_copyable_v<GridPosition>);
}
