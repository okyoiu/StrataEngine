#include "src/render/renderer.hpp"
#include "src/core/assert.hpp"

#include <array>

namespace tessera::render {

namespace {
    constexpr std::array<const char*, 4> kFontCandidates = {
        "/System/Library/Fonts/SFNSMono.ttf",
        "/System/Library/Fonts/Supplemental/Andale Mono.ttf",
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        "/opt/homebrew/opt/sdl2_ttf/share/sdl2_ttf/DejaVuSans.ttf",
    };
}

Renderer::Renderer(i32 width, i32 height, const char* title)
    : width_{width}, height_{height}
{
    TESSERA_ASSERT(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());

    window_ = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    TESSERA_ASSERT(window_ != nullptr, SDL_GetError());

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    TESSERA_ASSERT(renderer_ != nullptr, SDL_GetError());
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    if (TTF_Init() == 0) {
        for (const char* path : kFontCandidates) {
            font_ = TTF_OpenFont(path, 16);
            if (font_) break;
        }
    }
}

Renderer::~Renderer() {
    if (font_) TTF_CloseFont(font_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    TTF_Quit();
    SDL_Quit();
}

void Renderer::on_resize(i32 width, i32 height) {
    width_ = width;
    height_ = height;
}

void Renderer::clear() {
    SDL_SetRenderDrawColor(renderer_, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
    SDL_RenderClear(renderer_);
}

void Renderer::present() {
    SDL_RenderPresent(renderer_);
}

void Renderer::draw_world_tile(const Camera& cam, i32 gx, i32 gy, Color color, f32 inset) const {
    const f32 tile_px = cam.pixels_per_tile();
    const auto [sx, sy] = cam.world_to_screen(static_cast<f32>(gx) - 0.5f, static_cast<f32>(gy) - 0.5f);

    const f32 pad = tile_px * inset;
    const SDL_FRect rect{sx + pad, sy + pad, tile_px - pad * 2.0f, tile_px - pad * 2.0f};

    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(renderer_, &rect);
}

void Renderer::draw_direction_marker(const Camera& cam, i32 gx, i32 gy, sim::Direction dir) const {
    const f32 tile_px = cam.pixels_per_tile();
    const auto [cx, cy] = cam.world_to_screen(static_cast<f32>(gx), static_cast<f32>(gy));
    const f32 half = tile_px * 0.5f;
    const f32 marker = tile_px * 0.18f;

    f32 mx = cx;
    f32 my = cy;
    switch (dir) {
        case sim::Direction::North: my = cy - half + marker; break;
        case sim::Direction::South: my = cy + half - marker; break;
        case sim::Direction::East:  mx = cx + half - marker; break;
        case sim::Direction::West:  mx = cx - half + marker; break;
    }

    const SDL_FRect rect{mx - marker * 0.5f, my - marker * 0.5f, marker, marker};
    SDL_SetRenderDrawColor(renderer_, DIRECTION_MARKER_COLOR.r, DIRECTION_MARKER_COLOR.g,
                           DIRECTION_MARKER_COLOR.b, DIRECTION_MARKER_COLOR.a);
    SDL_RenderFillRectF(renderer_, &rect);
}

void Renderer::draw_item_dot(const Camera& cam, i32 gx, i32 gy, Color color) const {
    const f32 tile_px = cam.pixels_per_tile();
    const auto [cx, cy] = cam.world_to_screen(static_cast<f32>(gx), static_cast<f32>(gy));
    const f32 r = tile_px * 0.16f;
    const SDL_FRect rect{cx - r, cy - r, r * 2.0f, r * 2.0f};

    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(renderer_, &rect);
}

void Renderer::draw_grid_lines(const Camera& cam) const {
    const f32 tile_px = cam.pixels_per_tile();
    if (tile_px < 8.0f) return; // too zoomed out to read gridlines usefully

    SDL_SetRenderDrawColor(renderer_, GRID_LINE_COLOR.r, GRID_LINE_COLOR.g, GRID_LINE_COLOR.b, GRID_LINE_COLOR.a);

    const auto [world_left, world_top] = cam.screen_to_world(0.0f, 0.0f);
    const auto [world_right, world_bottom] = cam.screen_to_world(
        static_cast<f32>(width_), static_cast<f32>(height_));

    for (i32 gx = world_left - 1; gx <= world_right + 1; ++gx) {
        const auto [sx, sy_top] = cam.world_to_screen(static_cast<f32>(gx) - 0.5f, static_cast<f32>(world_top) - 1.0f);
        const auto [sx2, sy_bottom] = cam.world_to_screen(static_cast<f32>(gx) - 0.5f, static_cast<f32>(world_bottom) + 1.0f);
        SDL_RenderDrawLineF(renderer_, sx, sy_top, sx2, sy_bottom);
    }
    for (i32 gy = world_top - 1; gy <= world_bottom + 1; ++gy) {
        const auto [sx_left, sy] = cam.world_to_screen(static_cast<f32>(world_left) - 1.0f, static_cast<f32>(gy) - 0.5f);
        const auto [sx_right, sy2] = cam.world_to_screen(static_cast<f32>(world_right) + 1.0f, static_cast<f32>(gy) - 0.5f);
        SDL_RenderDrawLineF(renderer_, sx_left, sy, sx_right, sy2);
    }
}

void Renderer::draw_text(i32 screen_x, i32 screen_y, const std::string& text, Color color) const {
    if (!font_ || text.empty()) return;

    const SDL_Color sdl_color{color.r, color.g, color.b, color.a};
    SDL_Surface* surface = TTF_RenderText_Blended(font_, text.c_str(), sdl_color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture) {
        const SDL_Rect dst{screen_x, screen_y, surface->w, surface->h};
        SDL_RenderCopy(renderer_, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

auto Renderer::text_width(const std::string& text) const -> i32 {
    if (!font_ || text.empty()) return 0;
    int w = 0, h = 0;
    TTF_SizeText(font_, text.c_str(), &w, &h);
    return w;
}
}
