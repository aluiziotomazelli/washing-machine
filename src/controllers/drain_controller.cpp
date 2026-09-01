#include "drain_controller.hpp"

namespace controllers {

DrainController::DrainController(
    hal::ITimerHAL& timer_hal,
    hal::IDigitalOutput& drain_pump,
    hal::IWaterLevelSensor& water_sensor,
    uint32_t timeout_ms
)
    : timer_hal_(timer_hal)
    , drain_pump_(drain_pump)
    , water_sensor_(water_sensor)
    , timeout_ms_(timeout_ms)
{
}

void DrainController::start(uint32_t bleed_duration_ms)
{
    bleed_duration_ms_ = bleed_duration_ms;
    start_time_ms_ = timer_hal_.get_time_ms();
    bleed_start_ms_ = 0;
    drain_elapsed_before_pause_ms_ = 0;
    bleed_elapsed_before_pause_ms_ = 0;

    bleeding_phase_ = false;
    is_active_ = true;
    is_paused_ = false;
    is_finished_ = false;
    has_error_ = false;

    drain_pump_.turn_on();
}

void DrainController::update()
{
    if (!is_active_ || is_paused_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();

    if (!bleeding_phase_) {
        if (water_sensor_.is_empty()) {
            bleeding_phase_ = true;
            bleed_start_ms_ = now;
            bleed_elapsed_before_pause_ms_ = 0;
        } else {
            uint32_t total_drain_elapsed = drain_elapsed_before_pause_ms_ + (now - start_time_ms_);
            if (total_drain_elapsed >= timeout_ms_) {
                stop();
                has_error_ = true;
                return;
            }
        }
    } else {
        // Bleeding phase: pump remaining water until bleed duration expires
        uint32_t total_bleed_elapsed = bleed_elapsed_before_pause_ms_ + (now - bleed_start_ms_);
        if (total_bleed_elapsed >= bleed_duration_ms_) {
            stop();
            is_finished_ = true;
        }
    }
}

void DrainController::pause()
{
    if (!is_active_ || is_paused_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    if (!bleeding_phase_) {
        drain_elapsed_before_pause_ms_ += (now - start_time_ms_);
    } else {
        bleed_elapsed_before_pause_ms_ += (now - bleed_start_ms_);
    }

    drain_pump_.turn_off();
    is_paused_ = true;
}

void DrainController::resume()
{
    if (!is_active_ || !is_paused_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    if (!bleeding_phase_) {
        start_time_ms_ = now;
    } else {
        bleed_start_ms_ = now;
    }

    drain_pump_.turn_on();
    is_paused_ = false;
}

void DrainController::stop()
{
    drain_pump_.turn_off();
    is_active_ = false;
    is_paused_ = false;
}

} // namespace controllers
