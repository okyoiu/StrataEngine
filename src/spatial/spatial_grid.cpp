#include "src/spatial/spatial_grid.hpp"
#include "src/core/assert.hpp"

namespace tessera::spatial {

namespace {
    // 64-bit integer mix (Murmur3 finalizer / splitmix64 style)
    inline u64 mix64(u64 x) {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ULL;
        x ^= x >> 33;
        return x;
    }

    inline usize next_power_of_two(usize n) {
        usize p = 1;
        while (p < n) p <<= 1;
        return p;
    }
}

u64 SpatialGrid::cell_key(i32 x, i32 y) {
    const u64 ux = static_cast<u64>(static_cast<u32>(x));
    const u64 uy = static_cast<u64>(static_cast<u32>(y));
    return mix64((ux << 32) | uy);
}

SpatialGrid::SpatialGrid(usize initial_capacity)
    : table_(next_power_of_two(initial_capacity < 8 ? 8 : initial_capacity))
    , count_{0}
    , tombstones_{0}
{}

void SpatialGrid::rebuild(usize new_capacity) {
    std::vector<Slot> old_table = std::move(table_);
    table_.assign(next_power_of_two(new_capacity), Slot{});
    tombstones_ = 0;

    const u64 mask = table_.size() - 1;
    for (auto& slot : old_table) {
        if (slot.state != SlotState::Occupied) continue;
        usize idx = static_cast<usize>(slot.key & mask);
        while (table_[idx].state == SlotState::Occupied) {
            idx = (idx + 1) & mask;
        }
        table_[idx] = slot;
    }
}

void SpatialGrid::insert(EntityID entity, i32 x, i32 y) {
    // keep occupied+tombstones below 60% load for short probe chains
    if ((count_ + tombstones_ + 1) * 5 >= table_.size() * 3) {
        rebuild(table_.size() * 2);
    }

    const u64 key = cell_key(x, y);
    const u64 mask = table_.size() - 1;
    usize idx = static_cast<usize>(key & mask);
    usize first_tombstone = table_.size(); // sentinel: none found yet

    while (table_[idx].state != SlotState::Empty) {
        TESSERA_ASSERT(!(table_[idx].state == SlotState::Occupied && table_[idx].key == key),
                       "SpatialGrid: cell already occupied");
        if (table_[idx].state == SlotState::Deleted && first_tombstone == table_.size()) {
            first_tombstone = idx;
        }
        idx = (idx + 1) & mask;
    }

    const usize target = (first_tombstone != table_.size()) ? first_tombstone : idx;
    if (table_[target].state == SlotState::Deleted) --tombstones_;
    table_[target] = Slot{key, entity, SlotState::Occupied};
    entity_to_cell_[entity_index(entity)] = key;
    ++count_;
}

void SpatialGrid::remove(EntityID entity) {
    const u32 idx_key = entity_index(entity);
    auto it = entity_to_cell_.find(idx_key);
    TESSERA_ASSERT(it != entity_to_cell_.end(), "SpatialGrid: entity not present in grid");
    const u64 key = it->second;

    const u64 mask = table_.size() - 1;
    usize idx = static_cast<usize>(key & mask);
    while (!(table_[idx].state == SlotState::Occupied && table_[idx].key == key)) {
        idx = (idx + 1) & mask;
    }

    table_[idx].state = SlotState::Deleted;
    table_[idx].entity = NULL_ENTITY;
    entity_to_cell_.erase(it);
    --count_;
    ++tombstones_;
}

void SpatialGrid::update(EntityID entity, i32 x, i32 y) {
    remove(entity);
    insert(entity, x, y);
}

auto SpatialGrid::find(i32 x, i32 y) const -> EntityID {
    const u64 key = cell_key(x, y);
    const u64 mask = table_.size() - 1;
    usize idx = static_cast<usize>(key & mask);

    for (usize probes = 0; probes < table_.size(); ++probes) {
        if (table_[idx].state == SlotState::Empty) {
            return NULL_ENTITY;
        }
        if (table_[idx].state == SlotState::Occupied && table_[idx].key == key) {
            return table_[idx].entity;
        }
        idx = (idx + 1) & mask;
    }
    return NULL_ENTITY;
}

void SpatialGrid::query_radius(i32 center_x, i32 center_y, i32 radius, GridQueryBuffer& out) const {
    out.count = 0;
    for (i32 y = center_y - radius; y <= center_y + radius && out.count < GridQueryBuffer::CAPACITY; ++y) {
        for (i32 x = center_x - radius; x <= center_x + radius && out.count < GridQueryBuffer::CAPACITY; ++x) {
            const EntityID found = find(x, y);
            if (found != NULL_ENTITY) {
                out.entities[out.count++] = found;
            }
        }
    }
}
}
