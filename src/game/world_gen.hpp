#pragma once
#include "src/core/types.hpp"
#include "src/ecs/registry.hpp"
#include "src/spatial/spatial_grid.hpp"
#include "src/simulation/components.hpp"

#include <array>
#include <random>

namespace tessera::game {

// The playable world is a large but bounded square. Goals scale forever even
// though the map doesn't need to, since resource nodes never deplete.
inline constexpr i32 WORLD_HALF_EXTENT = 60;

inline void generate_world(ecs::Registry& registry, spatial::SpatialGrid& terrain, u32 seed = 1337) {
    std::mt19937 rng{seed};
    std::uniform_int_distribution<i32> coord_dist(-WORLD_HALF_EXTENT + 5, WORLD_HALF_EXTENT - 5);
    std::uniform_int_distribution<i32> cluster_size_dist(10, 24);
    std::uniform_int_distribution<i32> step_dist(0, 3);

    constexpr std::array<sim::ResourceType, 5> types = {
        sim::ResourceType::Iron, sim::ResourceType::Copper, sim::ResourceType::Coal,
        sim::ResourceType::Stone, sim::ResourceType::Gold,
    };
    constexpr i32 CLUSTERS_PER_TYPE = 4;

    for (const sim::ResourceType type : types) {
        for (i32 c = 0; c < CLUSTERS_PER_TYPE; ++c) {
            i32 x = coord_dist(rng);
            i32 y = coord_dist(rng);
            const i32 tiles = cluster_size_dist(rng);

            for (i32 i = 0; i < tiles; ++i) {
                if (terrain.find(x, y) == NULL_ENTITY) {
                    const EntityID node = registry.create();
                    registry.add<sim::GridPosition>(node, sim::GridPosition{x, y});
                    registry.add<sim::ResourceNode>(node, sim::ResourceNode{type});
                    terrain.insert(node, x, y);
                }
                // random walk so each patch reads as an organic blob, not a square
                switch (step_dist(rng)) {
                    case 0: ++x; break;
                    case 1: --x; break;
                    case 2: ++y; break;
                    default: --y; break;
                }
            }
        }
    }
}
}
