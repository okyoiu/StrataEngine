#pragma once
#include "src/core/types.hpp"
#include "src/ecs/registry.hpp"
#include "src/spatial/spatial_grid.hpp"
#include "src/simulation/components.hpp"
#include "src/simulation/tick_scheduler.hpp"
#include "src/game/economy.hpp"
#include "src/game/stats.hpp"
#include "src/game/objectives.hpp"
#include "src/render/camera.hpp"
#include "src/render/renderer.hpp"
#include "src/app/input_handler.hpp"

namespace tessera::app {

// Owns every subsystem and drives the fixed-timestep game loop: poll input,
// advance simulation ticks, render a frame. This is the whole game.
class App {
public:
    App();
    void run();

private:
    void poll_events();
    void handle_requests();
    [[nodiscard]] auto can_place(i32 gx, i32 gy, sim::BuildingKind kind) -> bool;
    void try_place(i32 gx, i32 gy, sim::BuildingKind kind, sim::Direction facing);
    void try_bulldoze(i32 gx, i32 gy);
    void tick();
    void render_frame();
    void draw_hud();

    ecs::Registry registry_;
    spatial::SpatialGrid terrain_grid_;
    spatial::SpatialGrid building_grid_;
    game::Economy economy_;
    game::Stats stats_;
    game::ObjectiveManager objectives_;

    render::Renderer renderer_;
    render::Camera camera_;
    InputHandler input_;
    sim::TickScheduler scheduler_;
};
}
