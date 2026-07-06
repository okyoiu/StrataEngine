#pragma once
#include "src/core/types.hpp"
#include "src/game/economy.hpp"
#include "src/game/stats.hpp"

#include <cmath>
#include <string>
#include <vector>

namespace tessera::game {

enum class ObjectiveKind : u8 {
    EarnMoney,
    SellItems,
    PlaceMiners,
    PlaceBelts,
    PlaceInserters,
    PlaceSellers,
};

struct Objective {
    ObjectiveKind kind;
    std::string description;
    f64 target;
    f64 reward;
};

// Tracks the player's current goal. Ships with a curated tutorial-style chain
// that introduces every mechanic; once that's exhausted it generates
// procedurally scaling goals forever, so there is always a next objective.
class ObjectiveManager {
public:
    ObjectiveManager() {
        curated_ = build_curated_chain();
        current_ = curated_[0];
    }

    // Call once per tick (or on every relevant state change). Returns the
    // reward money to add to the wallet if the current objective just
    // completed this call, otherwise 0.
    [[nodiscard]] auto update(const Economy& economy, const Stats& stats) -> f64 {
        if (progress_value(economy, stats) < current_.target) {
            return 0.0;
        }
        const f64 reward = current_.reward;
        ++completed_count_;
        advance();
        return reward;
    }

    [[nodiscard]] auto current() const noexcept -> const Objective& { return current_; }
    [[nodiscard]] auto completed_count() const noexcept -> u32 { return completed_count_; }

    [[nodiscard]] auto progress_value(const Economy& economy, const Stats& stats) const noexcept -> f64 {
        switch (current_.kind) {
            case ObjectiveKind::EarnMoney:      return economy.lifetime_earnings;
            case ObjectiveKind::SellItems:      return static_cast<f64>(economy.total_items_sold());
            case ObjectiveKind::PlaceMiners:    return static_cast<f64>(stats.miners_placed);
            case ObjectiveKind::PlaceBelts:     return static_cast<f64>(stats.belts_placed);
            case ObjectiveKind::PlaceInserters: return static_cast<f64>(stats.inserters_placed);
            case ObjectiveKind::PlaceSellers:   return static_cast<f64>(stats.sellers_placed);
        }
        return 0.0;
    }

private:
    std::vector<Objective> curated_;
    usize curated_index_ = 0;
    u32 procedural_tier_ = 0;
    Objective current_{};
    u32 completed_count_ = 0;

    void advance() {
        ++curated_index_;
        if (curated_index_ < curated_.size()) {
            current_ = curated_[curated_index_];
        } else {
            current_ = make_procedural(procedural_tier_++);
        }
    }

    static auto make_procedural(u32 tier) -> Objective {
        // Alternates between money and throughput goals, each growing
        // exponentially, so late-game targets keep climbing without limit.
        const f64 growth = 1.35;
        const f64 scale = std::pow(growth, static_cast<f64>(tier));

        if (tier % 2 == 0) {
            const f64 target = 1000.0 * scale;
            return Objective{
                ObjectiveKind::EarnMoney,
                "Earn $" + format_number(target) + " lifetime",
                target,
                target * 0.15,
            };
        }
        const f64 target = 200.0 * scale;
        return Objective{
            ObjectiveKind::SellItems,
            "Sell " + format_number(target) + " items total",
            target,
            target * 2.0,
        };
    }

    static auto format_number(f64 value) -> std::string {
        const u64 rounded = static_cast<u64>(value + 0.5);
        return std::to_string(rounded);
    }

    static auto build_curated_chain() -> std::vector<Objective> {
        return {
            {ObjectiveKind::PlaceMiners,    "Place your first Miner on an ore patch", 1,   20.0},
            {ObjectiveKind::PlaceBelts,     "Lay a Belt to carry ore away",           1,   20.0},
            {ObjectiveKind::PlaceSellers,   "Place a Seller to turn ore into money",  1,   30.0},
            {ObjectiveKind::SellItems,      "Sell 10 items",                         10,   50.0},
            {ObjectiveKind::EarnMoney,      "Earn $200 lifetime",                   200,  100.0},
            {ObjectiveKind::PlaceInserters, "Place an Inserter",                      1,   50.0},
            {ObjectiveKind::PlaceMiners,    "Place 3 Miners",                         3,  150.0},
            {ObjectiveKind::SellItems,      "Sell 100 items",                       100,  300.0},
            {ObjectiveKind::EarnMoney,      "Earn $1,000 lifetime",               1000,  500.0},
            {ObjectiveKind::SellItems,      "Sell 500 items",                       500, 1000.0},
        };
    }
};
}
