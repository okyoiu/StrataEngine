#pragma once
#include "src/core/types.hpp"

#include <cmath>
#include <utility>

namespace tessera::render {

// Converts between world tile coordinates and screen pixel coordinates.
// The camera's (x_, y_) is the world-space tile currently centered in the viewport.
class Camera {
public:
    Camera(i32 viewport_width, i32 viewport_height) noexcept
        : viewport_w_{viewport_width}, viewport_h_{viewport_height} {}

    void pan(f32 dx_tiles, f32 dy_tiles) noexcept { x_ += dx_tiles; y_ += dy_tiles; }

    void zoom_by(f32 factor) noexcept {
        pixels_per_tile_ *= factor;
        if (pixels_per_tile_ < MIN_ZOOM) pixels_per_tile_ = MIN_ZOOM;
        if (pixels_per_tile_ > MAX_ZOOM) pixels_per_tile_ = MAX_ZOOM;
    }

    void set_viewport(i32 w, i32 h) noexcept { viewport_w_ = w; viewport_h_ = h; }

    [[nodiscard]] auto world_to_screen(f32 wx, f32 wy) const noexcept -> std::pair<f32, f32> {
        const f32 sx = (wx - x_) * pixels_per_tile_ + static_cast<f32>(viewport_w_) * 0.5f;
        const f32 sy = (wy - y_) * pixels_per_tile_ + static_cast<f32>(viewport_h_) * 0.5f;
        return {sx, sy};
    }

    [[nodiscard]] auto screen_to_world(f32 sx, f32 sy) const noexcept -> std::pair<i32, i32> {
        const f32 wx = (sx - static_cast<f32>(viewport_w_) * 0.5f) / pixels_per_tile_ + x_;
        const f32 wy = (sy - static_cast<f32>(viewport_h_) * 0.5f) / pixels_per_tile_ + y_;
        return {static_cast<i32>(std::floor(wx + 0.5f)), static_cast<i32>(std::floor(wy + 0.5f))};
    }

    [[nodiscard]] auto pixels_per_tile() const noexcept -> f32 { return pixels_per_tile_; }
    [[nodiscard]] auto viewport_width() const noexcept -> i32 { return viewport_w_; }
    [[nodiscard]] auto viewport_height() const noexcept -> i32 { return viewport_h_; }

private:
    static constexpr f32 MIN_ZOOM = 4.0f;
    static constexpr f32 MAX_ZOOM = 96.0f;

    i32 viewport_w_;
    i32 viewport_h_;
    f32 x_ = 0.0f;
    f32 y_ = 0.0f;
    f32 pixels_per_tile_ = 24.0f;
};
}
