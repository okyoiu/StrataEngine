#pragma once
#include "src/ecs/registry.hpp"
#include "src/ecs/view.hpp"
#include "src/spatial/spatial_grid.hpp"
#include "src/simulation/components.hpp"
#include "src/simulation/item_transfer.hpp"
#include "src/game/economy.hpp"

namespace tessera::sim {

inline void update_belts(ecs::Registry& registry, const spatial::SpatialGrid& buildings, game::Economy& economy) {
    ecs::make_view<BeltLane, GridPosition>(registry).each(
        [&](EntityID /*entity*/, BeltLane& belt, GridPosition& pos) {
            if (!belt.has_item) return;
            if (belt.progress < BELT_TICKS_PER_MOVE) {
                ++belt.progress;
                if (belt.progress < BELT_TICKS_PER_MOVE) return;
            }

            const GridPosition target_pos = pos + direction_delta(belt.facing);
            const EntityID target = buildings.find(target_pos.x, target_pos.y);
            if (tile_can_accept(registry, target)) {
                tile_deposit(registry, economy, target, belt.item_type);
                belt.has_item = false;
                belt.progress = 0;
            }
            // else: blocked — item waits at full progress and retries next tick
        });
}
}
