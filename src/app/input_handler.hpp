#pragma once
#include "src/core/types.hpp"
#include "src/render/camera.hpp"
#include "src/simulation/components.hpp"

#include <SDL2/SDL.h>
#include <optional>
#include <utility>

namespace tessera::app {

struct PlacementRequest {
    i32 gx, gy;
    sim::BuildingKind kind;
    sim::Direction facing;
};

struct BulldozeRequest {
    i32 gx, gy;
};

// Translates raw SDL input into camera moves, building selection, and
// place/bulldoze requests the App can act on each frame.
class InputHandler {
public:
    void handle_event(const SDL_Event& event, render::Camera& camera);
    void update_continuous(f32 dt, render::Camera& camera);

    [[nodiscard]] auto should_quit() const noexcept -> bool { return quit_; }
    [[nodiscard]] auto selected_kind() const noexcept -> sim::BuildingKind { return selected_kind_; }
    [[nodiscard]] auto selected_facing() const noexcept -> sim::Direction { return selected_facing_; }
    [[nodiscard]] auto hover_tile() const noexcept -> std::pair<i32, i32> { return {hover_x_, hover_y_}; }

    [[nodiscard]] auto take_placement_request() -> std::optional<PlacementRequest>;
    [[nodiscard]] auto take_bulldoze_request() -> std::optional<BulldozeRequest>;

private:
    bool quit_ = false;
    sim::BuildingKind selected_kind_ = sim::BuildingKind::Miner;
    sim::Direction selected_facing_ = sim::Direction::East;
    i32 hover_x_ = 0;
    i32 hover_y_ = 0;

    std::optional<PlacementRequest> pending_placement_;
    std::optional<BulldozeRequest> pending_bulldoze_;
};
}
