#pragma once
#include "src/core/types.hpp"

namespace tessera::game {

// Lifetime counters that objectives are evaluated against. Never reset.
struct Stats {
    u32 miners_placed = 0;
    u32 belts_placed = 0;
    u32 inserters_placed = 0;
    u32 sellers_placed = 0;

    [[nodiscard]] auto buildings_placed() const noexcept -> u32 {
        return miners_placed + belts_placed + inserters_placed + sellers_placed;
    }
};
}
