#pragma once
#include "src/ecs/registry.hpp"
#include "src/ecs/view.hpp"
#include "src/spatial/spatial_grid.hpp"
#include "src/simulation/components.hpp"
#include "src/simulation/item_transfer.hpp"
#include "src/game/economy.hpp"

namespace tessera::sim {

inline void update_inserters(ecs::Registry& registry, const spatial::SpatialGrid& buildings, game::Economy& economy) {
    ecs::make_view<Inserter, GridPosition>(registry).each(
        [&](EntityID /*entity*/, Inserter& inserter, GridPosition& pos) {
            ++inserter.progress;
            if (inserter.progress < INSERTER_TICKS_PER_MOVE) return;
            inserter.progress = 0;

            const GridPosition source_pos = pos + direction_delta(opposite(inserter.facing));
            const GridPosition target_pos = pos + direction_delta(inserter.facing);
            const EntityID source = buildings.find(source_pos.x, source_pos.y);
            const EntityID target = buildings.find(target_pos.x, target_pos.y);

            ResourceType carried;
            if (!tile_has_pickup(registry, source, carried)) return;
            if (!tile_can_accept(registry, target)) return;

            tile_withdraw(registry, source);
            tile_deposit(registry, economy, target, carried);
        });
}
}
