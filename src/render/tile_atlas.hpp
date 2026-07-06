#pragma once
#include "src/core/types.hpp"
#include "src/simulation/components.hpp"

namespace tessera::render {

struct Color {
    u8 r, g, b, a = 255;
};

[[nodiscard]] constexpr Color resource_color(sim::ResourceType type) noexcept {
    switch (type) {
        case sim::ResourceType::Iron:   return {176, 176, 190, 255};
        case sim::ResourceType::Copper: return {201, 111, 66, 255};
        case sim::ResourceType::Coal:   return {50, 50, 54, 255};
        case sim::ResourceType::Stone:  return {150, 140, 126, 255};
        case sim::ResourceType::Gold:   return {230, 190, 60, 255};
        default:                        return {255, 0, 255, 255};
    }
}

[[nodiscard]] constexpr Color building_color(sim::BuildingKind kind) noexcept {
    switch (kind) {
        case sim::BuildingKind::Miner:    return {90, 140, 220, 255};
        case sim::BuildingKind::Belt:     return {150, 140, 70, 255};
        case sim::BuildingKind::Inserter: return {200, 90, 200, 255};
        case sim::BuildingKind::Seller:   return {70, 200, 100, 255};
        default:                          return {255, 255, 255, 255};
    }
}

inline constexpr Color BACKGROUND_COLOR{24, 28, 24, 255};
inline constexpr Color TERRAIN_EMPTY_COLOR{34, 40, 34, 255};
inline constexpr Color GRID_LINE_COLOR{46, 54, 46, 255};
inline constexpr Color ITEM_DOT_COLOR{250, 240, 210, 255};
inline constexpr Color DIRECTION_MARKER_COLOR{20, 20, 20, 220};
inline constexpr Color GHOST_VALID_COLOR{120, 255, 120, 160};
inline constexpr Color GHOST_INVALID_COLOR{255, 90, 90, 160};
inline constexpr Color HUD_TEXT_COLOR{240, 240, 235, 255};
inline constexpr Color HUD_ACCENT_COLOR{255, 210, 90, 255};
}
