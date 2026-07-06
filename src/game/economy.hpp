#pragma once
#include "src/core/types.hpp"
#include "src/simulation/components.hpp"

#include <array>

namespace tessera::game {

// Build costs — what it costs to place each building kind.
[[nodiscard]] constexpr f64 build_cost(sim::BuildingKind kind) noexcept {
    switch (kind) {
        case sim::BuildingKind::Miner:    return 15.0;
        case sim::BuildingKind::Belt:     return 2.0;
        case sim::BuildingKind::Inserter: return 10.0;
        case sim::BuildingKind::Seller:   return 25.0;
        default:                          return 0.0;
    }
}

// The player's wallet. Every unit sold through a Seller flows through here —
// this is the game's sole source of income.
struct Economy {
    f64 money = 75.0;
    f64 lifetime_earnings = 0.0;
    std::array<u64, sim::RESOURCE_TYPE_COUNT> total_sold{};

    void sell(sim::ResourceType type, u32 amount) noexcept {
        const f64 value = sim::resource_price(type) * static_cast<f64>(amount);
        money += value;
        lifetime_earnings += value;
        total_sold[static_cast<usize>(type)] += amount;
    }

    [[nodiscard]] auto total_items_sold() const noexcept -> u64 {
        u64 sum = 0;
        for (const u64 count : total_sold) sum += count;
        return sum;
    }

    [[nodiscard]] auto can_afford(f64 cost) const noexcept -> bool {
        return money >= cost;
    }

    // Returns true and deducts the cost if affordable; otherwise leaves money untouched.
    [[nodiscard]] auto try_spend(f64 cost) noexcept -> bool {
        if (!can_afford(cost)) return false;
        money -= cost;
        return true;
    }
};
}
