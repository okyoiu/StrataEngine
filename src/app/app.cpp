#include "src/app/app.hpp"

#include "src/simulation/miner_system.hpp"
#include "src/simulation/belt_system.hpp"
#include "src/simulation/inserter_system.hpp"
#include "src/game/world_gen.hpp"
#include "src/render/tile_atlas.hpp"

#include <SDL2/SDL.h>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace tessera::app {

App::App()
    : registry_{}
    , terrain_grid_{8192}
    , building_grid_{8192}
    , renderer_{1280, 800, "Tessera \xe2\x80\x94 Factory Automation"}
    , camera_{1280, 800}
{
    game::generate_world(registry_, terrain_grid_);

    std::cout << std::unitbuf; // flush eagerly so console status is visible in real time
    std::cout << "=== Tessera ===\n"
              << "Controls: [1-4] select building   R: rotate   LMB: place   RMB: bulldoze\n"
              << "          WASD/Arrows: pan camera   Mouse wheel or +/-: zoom   Esc: quit\n"
              << "Starting money: $" << economy_.money << "\n"
              << "First objective: " << objectives_.current().description << "\n";
    if (!renderer_.has_font()) {
        std::cout << "(no system font found -- HUD text disabled; status prints here instead)\n";
    }
}

void App::poll_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        input_.handle_event(event, camera_);
    }
}

void App::handle_requests() {
    if (auto req = input_.take_placement_request()) {
        try_place(req->gx, req->gy, req->kind, req->facing);
    }
    if (auto req = input_.take_bulldoze_request()) {
        try_bulldoze(req->gx, req->gy);
    }
}

auto App::can_place(i32 gx, i32 gy, sim::BuildingKind kind) -> bool {
    if (building_grid_.find(gx, gy) != NULL_ENTITY) return false;

    if (kind == sim::BuildingKind::Miner) {
        const EntityID terrain_entity = terrain_grid_.find(gx, gy);
        if (terrain_entity == NULL_ENTITY || !registry_.has<sim::ResourceNode>(terrain_entity)) {
            return false;
        }
    }

    return economy_.can_afford(game::build_cost(kind));
}

void App::try_place(i32 gx, i32 gy, sim::BuildingKind kind, sim::Direction facing) {
    if (!can_place(gx, gy, kind)) return;
    if (!economy_.try_spend(game::build_cost(kind))) return;

    const EntityID entity = registry_.create();
    registry_.add<sim::GridPosition>(entity, sim::GridPosition{gx, gy});
    registry_.add<sim::Building>(entity, sim::Building{kind});

    switch (kind) {
        case sim::BuildingKind::Miner: {
            const EntityID terrain_entity = terrain_grid_.find(gx, gy);
            const auto& node = registry_.get<sim::ResourceNode>(terrain_entity);
            registry_.add<sim::Miner>(entity, sim::Miner{facing, node.type, 0, 0});
            ++stats_.miners_placed;
            break;
        }
        case sim::BuildingKind::Belt:
            registry_.add<sim::BeltLane>(entity, sim::BeltLane{facing, false, sim::ResourceType::Iron, 0});
            ++stats_.belts_placed;
            break;
        case sim::BuildingKind::Inserter:
            registry_.add<sim::Inserter>(entity, sim::Inserter{facing, 0});
            ++stats_.inserters_placed;
            break;
        case sim::BuildingKind::Seller:
            registry_.add<sim::Seller>(entity, sim::Seller{0});
            ++stats_.sellers_placed;
            break;
        default:
            break;
    }

    building_grid_.insert(entity, gx, gy);
}

void App::try_bulldoze(i32 gx, i32 gy) {
    const EntityID entity = building_grid_.find(gx, gy);
    if (entity == NULL_ENTITY) return;
    building_grid_.remove(entity);
    registry_.destroy(entity);
}

void App::tick() {
    sim::update_miners(registry_, building_grid_, economy_);
    sim::update_belts(registry_, building_grid_, economy_);
    sim::update_inserters(registry_, building_grid_, economy_);

    const f64 reward = objectives_.update(economy_, stats_);
    if (reward > 0.0) {
        economy_.money += reward;
        std::cout << "[Objective complete #" << objectives_.completed_count() << "] +$"
                   << std::fixed << std::setprecision(2) << reward
                   << "  -- next: " << objectives_.current().description << "\n";
    }
}

void App::render_frame() {
    renderer_.clear();

    const auto [left, top] = camera_.screen_to_world(0.0f, 0.0f);
    const auto [right, bottom] = camera_.screen_to_world(
        static_cast<f32>(renderer_.width()), static_cast<f32>(renderer_.height()));

    for (i32 gy = top - 1; gy <= bottom + 1; ++gy) {
        for (i32 gx = left - 1; gx <= right + 1; ++gx) {
            renderer_.draw_world_tile(camera_, gx, gy, render::TERRAIN_EMPTY_COLOR, 0.02f);

            const EntityID terrain_entity = terrain_grid_.find(gx, gy);
            const EntityID building_entity = building_grid_.find(gx, gy);

            if (terrain_entity != NULL_ENTITY && building_entity == NULL_ENTITY) {
                const auto& node = registry_.get<sim::ResourceNode>(terrain_entity);
                renderer_.draw_world_tile(camera_, gx, gy, render::resource_color(node.type), 0.22f);
            }

            if (building_entity != NULL_ENTITY) {
                const auto& building = registry_.get<sim::Building>(building_entity);
                renderer_.draw_world_tile(camera_, gx, gy, render::building_color(building.kind), 0.08f);

                switch (building.kind) {
                    case sim::BuildingKind::Miner: {
                        const auto& miner = registry_.get<sim::Miner>(building_entity);
                        renderer_.draw_direction_marker(camera_, gx, gy, miner.facing);
                        if (miner.buffer > 0) {
                            renderer_.draw_item_dot(camera_, gx, gy, render::resource_color(miner.type));
                        }
                        break;
                    }
                    case sim::BuildingKind::Belt: {
                        const auto& belt = registry_.get<sim::BeltLane>(building_entity);
                        renderer_.draw_direction_marker(camera_, gx, gy, belt.facing);
                        if (belt.has_item) {
                            renderer_.draw_item_dot(camera_, gx, gy, render::resource_color(belt.item_type));
                        }
                        break;
                    }
                    case sim::BuildingKind::Inserter: {
                        const auto& inserter = registry_.get<sim::Inserter>(building_entity);
                        renderer_.draw_direction_marker(camera_, gx, gy, inserter.facing);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

    renderer_.draw_grid_lines(camera_);

    const auto [hover_x, hover_y] = input_.hover_tile();
    const sim::BuildingKind ghost_kind = input_.selected_kind();
    const bool valid = can_place(hover_x, hover_y, ghost_kind);
    renderer_.draw_world_tile(camera_, hover_x, hover_y,
        valid ? render::GHOST_VALID_COLOR : render::GHOST_INVALID_COLOR, 0.1f);
    if (ghost_kind != sim::BuildingKind::Seller) {
        renderer_.draw_direction_marker(camera_, hover_x, hover_y, input_.selected_facing());
    }

    draw_hud();
    renderer_.present();
}

void App::draw_hud() {
    const i32 margin = 10;
    const i32 line_height = 20;
    i32 y = margin;

    std::ostringstream money_line;
    money_line << "Money: $" << std::fixed << std::setprecision(2) << economy_.money;
    renderer_.draw_text(margin, y, money_line.str(), render::HUD_ACCENT_COLOR);
    y += line_height;

    const auto& objective = objectives_.current();
    std::ostringstream obj_line;
    obj_line << "Objective: " << objective.description << "  ["
              << static_cast<u64>(objectives_.progress_value(economy_, stats_)) << " / "
              << static_cast<u64>(objective.target) << "]";
    renderer_.draw_text(margin, y, obj_line.str(), render::HUD_TEXT_COLOR);
    y += line_height;

    std::ostringstream completed_line;
    completed_line << "Objectives completed: " << objectives_.completed_count();
    renderer_.draw_text(margin, y, completed_line.str(), render::HUD_TEXT_COLOR);
    y += line_height;

    std::ostringstream controls_line;
    controls_line << "[1]Miner $" << static_cast<i32>(game::build_cost(sim::BuildingKind::Miner))
                  << "  [2]Belt $" << static_cast<i32>(game::build_cost(sim::BuildingKind::Belt))
                  << "  [3]Inserter $" << static_cast<i32>(game::build_cost(sim::BuildingKind::Inserter))
                  << "  [4]Seller $" << static_cast<i32>(game::build_cost(sim::BuildingKind::Seller))
                  << "   R:rotate  LMB:place  RMB:bulldoze  WASD:pan  wheel:zoom";
    renderer_.draw_text(margin, y, controls_line.str(), render::HUD_TEXT_COLOR);
}

void App::run() {
    u64 last_ticks = SDL_GetTicks64();
    f64 console_timer = 0.0;

    while (!input_.should_quit()) {
        const u64 now = SDL_GetTicks64();
        const f64 frame_dt = static_cast<f64>(now - last_ticks) / 1000.0;
        last_ticks = now;

        poll_events();
        input_.update_continuous(static_cast<f32>(frame_dt), camera_);
        handle_requests();

        scheduler_.begin_frame(frame_dt);
        while (scheduler_.consume_tick()) {
            tick();
        }

        render_frame();

        console_timer += frame_dt;
        if (console_timer >= 3.0) {
            console_timer = 0.0;
            std::cout << "[status] $" << std::fixed << std::setprecision(2) << economy_.money
                       << "  sold=" << economy_.total_items_sold()
                       << "  objective: " << objectives_.current().description
                       << " (" << static_cast<u64>(objectives_.progress_value(economy_, stats_))
                       << "/" << static_cast<u64>(objectives_.current().target) << ")\n";
        }
    }
}
}
