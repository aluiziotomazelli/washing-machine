#include "agitator.hpp"

namespace controllers {

Agitator::Agitator(hal::ITimerHAL& timer_hal, hal::IReversibleMotor& motor)
    : timer_hal_(timer_hal)
    , motor_(motor)
{
}

void Agitator::start(uint32_t duration_sec, uint16_t on_ms, uint16_t off_ms)
{
    duration_ms_ = duration_sec * 1000;
    on_ms_ = on_ms;
    off_ms_ = off_ms;

    step_start_ms_ = timer_hal_.get_time_ms();
    elapsed_before_pause_ms_ = 0;
    pause_start_ms_ = 0;

    stroke_start_ms_ = step_start_ms_;
    is_cw_stroke_ = true;
    stroke_motor_on_ = true;

    is_active_ = true;
    is_paused_ = false;
    is_finished_ = false;

    motor_.rotate_clockwise();
}

void Agitator::update()
{
    if (!is_active_ || is_paused_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    uint32_t total_elapsed = elapsed_before_pause_ms_ + (now - step_start_ms_);

    // Check if total agitation duration completed
    if (total_elapsed >= duration_ms_) {
        stop();
        is_finished_ = true;
        return;
    }

    // Alternating Stroke Machine
    uint32_t elapsed_in_stroke = now - stroke_start_ms_;
    if (stroke_motor_on_) {
        if (elapsed_in_stroke >= on_ms_) {
            motor_.stop();
            stroke_motor_on_ = false;
            stroke_start_ms_ = now;
        }
    } else {
        if (elapsed_in_stroke >= off_ms_) {
            is_cw_stroke_ = !is_cw_stroke_;
            stroke_motor_on_ = true;
            stroke_start_ms_ = now;
            if (is_cw_stroke_) {
                motor_.rotate_clockwise();
            } else {
                motor_.rotate_counter_clockwise();
            }
        }
    }
}

void Agitator::pause()
{
    if (!is_active_ || is_paused_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    pause_start_ms_ = now;
    elapsed_before_pause_ms_ += (pause_start_ms_ - step_start_ms_);

    motor_.stop();
    stroke_motor_on_ = false;
    is_paused_ = true;
}

void Agitator::resume()
{
    if (!is_active_ || !is_paused_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    step_start_ms_ = now;
    stroke_start_ms_ = now;
    is_paused_ = false;

    // Resume stroke
    stroke_motor_on_ = true;
    if (is_cw_stroke_) {
        motor_.rotate_clockwise();
    } else {
        motor_.rotate_counter_clockwise();
    }
}

void Agitator::stop()
{
    motor_.stop();
    is_active_ = false;
    is_paused_ = false;
    stroke_motor_on_ = false;
}

} // namespace controllers
