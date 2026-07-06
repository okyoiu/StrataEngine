#pragma once
#include "src/core/types.hpp"

namespace tessera::sim {

// Fixed-timestep accumulator. Call begin_frame() once per rendered frame with
// the real elapsed time, then call consume_tick() in a loop until it returns
// false to run the simulation at a constant tick rate regardless of frame rate.
class TickScheduler {
public:
    explicit TickScheduler(f64 ticks_per_second = 60.0, u32 max_ticks_per_frame = 5) noexcept
        : tick_dt_{1.0 / ticks_per_second}
        , max_ticks_per_frame_{max_ticks_per_frame}
    {}

    void begin_frame(f64 frame_dt) noexcept {
        accumulator_ += frame_dt;
        // Spiral-of-death guard: if we fall far enough behind (e.g. after a
        // debugger pause or OS hitch), drop the backlog instead of trying to
        // catch up tick-by-tick forever.
        const f64 max_accumulator = tick_dt_ * static_cast<f64>(max_ticks_per_frame_) * 2.0;
        if (accumulator_ > max_accumulator) accumulator_ = max_accumulator;
        ticks_this_frame_ = 0;
    }

    [[nodiscard]] auto consume_tick() noexcept -> bool {
        if (accumulator_ >= tick_dt_ && ticks_this_frame_ < max_ticks_per_frame_) {
            accumulator_ -= tick_dt_;
            ++ticks_this_frame_;
            return true;
        }
        return false;
    }

    [[nodiscard]] auto tick_dt() const noexcept -> f64 { return tick_dt_; }

private:
    f64 tick_dt_;
    f64 accumulator_ = 0.0;
    u32 max_ticks_per_frame_;
    u32 ticks_this_frame_ = 0;
};
}
