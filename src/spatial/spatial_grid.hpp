#pragma once
#include "src/core/types.hpp"
#include "src/ecs/entity.hpp"

#include <vector>
#include <unordered_map>

namespace tessera::spatial {

// Fixed-capacity output buffer for query_radius() -- avoids heap allocation
// on the query path. Callers own the storage; the grid only ever writes into it.
struct GridQueryBuffer {
    static constexpr usize CAPACITY = 256;
    EntityID entities[CAPACITY];
    usize count = 0;
};

// Flat open-addressing hash grid, one entity per cell. Each entity maps to
// exactly one integer (x, y) cell. Insert/remove/find are O(1) average case;
// query_radius is O(area) since it probes every cell in the bounding box.
class SpatialGrid {
public:
    explicit SpatialGrid(usize initial_capacity = 4096);

    void insert(EntityID entity, i32 x, i32 y);
    void remove(EntityID entity);
    void update(EntityID entity, i32 x, i32 y);

    [[nodiscard]] auto find(i32 x, i32 y) const -> EntityID;
    void query_radius(i32 center_x, i32 center_y, i32 radius, GridQueryBuffer& out) const;

    [[nodiscard]] auto size() const noexcept -> usize { return count_; }

private:
    enum class SlotState : u8 { Empty, Occupied, Deleted };
    struct Slot {
        u64 key = 0;
        EntityID entity = NULL_ENTITY;
        SlotState state = SlotState::Empty;
    };

    static u64 cell_key(i32 x, i32 y);
    void rebuild(usize new_capacity);

    std::vector<Slot> table_;
    // reverse map: entity index -> cell key, for O(1) removal without a linear scan
    std::unordered_map<u32, u64> entity_to_cell_;
    usize count_;
    usize tombstones_;
};
}
