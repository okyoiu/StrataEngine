#pragma once
#include "src/ecs/registry.hpp"
#include "src/ecs/view.hpp"
#include "src/spatial/spatial_grid.hpp"
#include "src/simulation/components.hpp"
#include "src/simulation/item_transfer.hpp"
#include "src/game/economy.hpp"

namespace tessera::sim {

inline void update_miners(ecs::Registry& registry, const spatial::SpatialGrid& buildings, game::Economy& economy) {
    ecs::make_view<Miner, GridPosition>(registry).each(
        [&](EntityID /*entity*/, Miner& miner, GridPosition& pos) {
            if (miner.buffer < MINER_MAX_BUFFER) {
                ++miner.progress;
                if (miner.progress >= MINER_TICKS_PER_ITEM) {
                    miner.progress = 0;
                    ++miner.buffer;
                }
            }

            if (miner.buffer == 0) return;

            const GridPosition target_pos = pos + direction_delta(miner.facing);
            const EntityID target = buildings.find(target_pos.x, target_pos.y);
            if (tile_can_accept(registry, target)) {
                tile_deposit(registry, economy, target, miner.type);
                --miner.buffer;
            }
        });
}
}
