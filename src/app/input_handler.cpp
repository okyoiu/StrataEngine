#include "src/app/input_handler.hpp"

namespace tessera::app {

void InputHandler::handle_event(const SDL_Event& event, render::Camera& camera) {
    switch (event.type) {
        case SDL_QUIT:
            quit_ = true;
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                camera.set_viewport(event.window.data1, event.window.data2);
            }
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: quit_ = true; break;
                case SDLK_1: selected_kind_ = sim::BuildingKind::Miner; break;
                case SDLK_2: selected_kind_ = sim::BuildingKind::Belt; break;
                case SDLK_3: selected_kind_ = sim::BuildingKind::Inserter; break;
                case SDLK_4: selected_kind_ = sim::BuildingKind::Seller; break;
                case SDLK_r: selected_facing_ = sim::rotate_cw(selected_facing_); break;
                case SDLK_EQUALS:
                case SDLK_KP_PLUS: camera.zoom_by(1.15f); break;
                case SDLK_MINUS:
                case SDLK_KP_MINUS: camera.zoom_by(1.0f / 1.15f); break;
                default: break;
            }
            break;

        case SDL_MOUSEWHEEL: {
            const f32 factor = event.wheel.y > 0 ? 1.1f : 1.0f / 1.1f;
            camera.zoom_by(factor);
            break;
        }

        case SDL_MOUSEMOTION: {
            const auto [gx, gy] = camera.screen_to_world(
                static_cast<f32>(event.motion.x), static_cast<f32>(event.motion.y));
            hover_x_ = gx;
            hover_y_ = gy;
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            const auto [gx, gy] = camera.screen_to_world(
                static_cast<f32>(event.button.x), static_cast<f32>(event.button.y));
            hover_x_ = gx;
            hover_y_ = gy;
            if (event.button.button == SDL_BUTTON_LEFT) {
                pending_placement_ = PlacementRequest{gx, gy, selected_kind_, selected_facing_};
            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                pending_bulldoze_ = BulldozeRequest{gx, gy};
            }
            break;
        }

        default:
            break;
    }
}

void InputHandler::update_continuous(f32 dt, render::Camera& camera) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    const f32 speed = 12.0f * dt;
    f32 dx = 0.0f;
    f32 dy = 0.0f;
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    dy -= speed;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  dy += speed;
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  dx -= speed;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) dx += speed;
    camera.pan(dx, dy);
}

auto InputHandler::take_placement_request() -> std::optional<PlacementRequest> {
    auto req = pending_placement_;
    pending_placement_.reset();
    return req;
}

auto InputHandler::take_bulldoze_request() -> std::optional<BulldozeRequest> {
    auto req = pending_bulldoze_;
    pending_bulldoze_.reset();
    return req;
}
}
