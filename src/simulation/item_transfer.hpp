#pragma once
#include "src/core/types.hpp"
#include "src/ecs/registry.hpp"
#include "src/simulation/components.hpp"
#include "src/game/economy.hpp"

namespace tessera::sim {

// Shared tile-interaction rules used by the miner, belt, and inserter systems.
// A "tile" here means whatever building entity a SpatialGrid lookup returns.

[[nodiscard]] inline bool tile_can_accept(ecs::Registry& registry, EntityID target) {
    if (target == NULL_ENTITY) return false;
    if (registry.has<BeltLane>(target)) return !registry.get<BeltLane>(target).has_item;
    if (registry.has<Seller>(target)) return true;
    return false;
}

inline void tile_deposit(ecs::Registry& registry, game::Economy& economy, EntityID target, ResourceType type) {
    if (registry.has<BeltLane>(target)) {
        auto& belt = registry.get<BeltLane>(target);
        belt.has_item = true;
        belt.item_type = type;
        belt.progress = 0;
        return;
    }
    if (registry.has<Seller>(target)) {
        auto& seller = registry.get<Seller>(target);
        ++seller.accepted_count;
        economy.sell(type, 1);
        return;
    }
}

[[nodiscard]] inline bool tile_has_pickup(ecs::Registry& registry, EntityID source, ResourceType& out_type) {
    if (source == NULL_ENTITY) return false;
    if (registry.has<BeltLane>(source)) {
        const auto& belt = registry.get<BeltLane>(source);
        if (!belt.has_item) return false;
        out_type = belt.item_type;
        return true;
    }
    if (registry.has<Miner>(source)) {
        const auto& miner = registry.get<Miner>(source);
        if (miner.buffer == 0) return false;
        out_type = miner.type;
        return true;
    }
    return false;
}

inline void tile_withdraw(ecs::Registry& registry, EntityID source) {
    if (registry.has<BeltLane>(source)) {
        auto& belt = registry.get<BeltLane>(source);
        belt.has_item = false;
        belt.progress = 0;
        return;
    }
    if (registry.has<Miner>(source)) {
        auto& miner = registry.get<Miner>(source);
        if (miner.buffer > 0) --miner.buffer;
        return;
    }
}
}
