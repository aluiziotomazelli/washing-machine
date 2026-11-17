#include "accel_timeline.hpp"

namespace controllers {

void AccelTimeline::start(uint32_t now_ms, uint32_t motor_on_target_ms, uint32_t wall_cap_ms)
{
    start_ms_ = now_ms;
    last_tick_ms_ = now_ms;
    motor_on_elapsed_ms_ = 0;
    motor_on_target_ms_ = motor_on_target_ms;
    wall_cap_ms_ = wall_cap_ms;
    motor_on_ = false;
    active_ = true;
}

void AccelTimeline::on_motor_on(uint32_t now_ms)
{
    if (motor_on_) {
        if (now_ms > last_tick_ms_) {
            motor_on_elapsed_ms_ += (now_ms - last_tick_ms_);
            last_tick_ms_ = now_ms;
        }
    }
    else {
        motor_on_ = true;
        last_tick_ms_ = now_ms;
    }
}

void AccelTimeline::on_motor_off(uint32_t now_ms)
{
    if (motor_on_) {
        if (now_ms > last_tick_ms_) {
            motor_on_elapsed_ms_ += (now_ms - last_tick_ms_);
        }
        motor_on_ = false;
    }
    last_tick_ms_ = now_ms;
}

bool AccelTimeline::is_complete(uint32_t now_ms) const
{
    if (!active_) {
        return false;
    }

    uint32_t total_on = motor_on_elapsed_ms_;
    if (motor_on_ && (now_ms > last_tick_ms_)) {
        total_on += (now_ms - last_tick_ms_);
    }

    uint32_t wall_elapsed = (now_ms >= start_ms_) ? (now_ms - start_ms_) : 0;

    return (total_on >= motor_on_target_ms_) || (wall_elapsed >= wall_cap_ms_);
}

void AccelTimeline::reset()
{
    start_ms_ = 0;
    last_tick_ms_ = 0;
    motor_on_elapsed_ms_ = 0;
    motor_on_target_ms_ = 15000;
    wall_cap_ms_ = 40000;
    motor_on_ = false;
    active_ = false;
}

} // namespace controllers
