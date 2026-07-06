#pragma once
#include "src/core/types.hpp"
#include "src/render/camera.hpp"
#include "src/render/tile_atlas.hpp"
#include "src/simulation/components.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>

namespace tessera::render {

// Thin SDL2 wrapper: owns the window/renderer/font and exposes tile-grid and
// text drawing primitives. No sprite atlas is used — buildings and resources
// are drawn as flat-shaded rectangles keyed by tile_atlas.hpp's color table.
class Renderer {
public:
    Renderer(i32 width, i32 height, const char* title);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void clear();
    void present();

    void draw_world_tile(const Camera& cam, i32 gx, i32 gy, Color color, f32 inset = 0.06f) const;
    void draw_direction_marker(const Camera& cam, i32 gx, i32 gy, sim::Direction dir) const;
    void draw_item_dot(const Camera& cam, i32 gx, i32 gy, Color color) const;
    void draw_grid_lines(const Camera& cam) const;

    void draw_text(i32 screen_x, i32 screen_y, const std::string& text, Color color) const;
    [[nodiscard]] auto text_width(const std::string& text) const -> i32;

    void on_resize(i32 width, i32 height);

    [[nodiscard]] auto width() const noexcept -> i32 { return width_; }
    [[nodiscard]] auto height() const noexcept -> i32 { return height_; }
    [[nodiscard]] auto has_font() const noexcept -> bool { return font_ != nullptr; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    TTF_Font* font_ = nullptr;
    i32 width_;
    i32 height_;
};
}
