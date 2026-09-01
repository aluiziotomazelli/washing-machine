#include "fill_controller.hpp"

namespace controllers {

FillController::FillController(
    hal::ITimerHAL& timer_hal,
    hal::IDigitalOutput& valve_main,
    hal::IDigitalOutput& valve_softener,
    hal::IWaterLevelSensor& water_sensor,
    uint32_t timeout_ms
)
    : timer_hal_(timer_hal)
    , valve_main_(valve_main)
    , valve_softener_(valve_softener)
    , water_sensor_(water_sensor)
    , timeout_ms_(timeout_ms)
{
}

void FillController::start(domain::WaterLevel target, bool use_softener)
{
    target_level_ = target;
    use_softener_ = use_softener;
    start_time_ms_ = timer_hal_.get_time_ms();
    elapsed_before_pause_ms_ = 0;

    is_active_ = true;
    is_paused_ = false;
    is_finished_ = false;
    has_error_ = false;

    valve_main_.turn_on();
    if (use_softener_) {
        valve_softener_.turn_on();
    } else {
        valve_softener_.turn_off();
    }
}

void FillController::update()
{
    if (!is_active_ || is_paused_) {
        return;
    }

    // Check if target level reached
    if (water_sensor_.is_level_reached(target_level_)) {
        stop();
        is_finished_ = true;
        return;
    }

    // Check timeout
    uint32_t now = timer_hal_.get_time_ms();
    uint32_t total_elapsed = elapsed_before_pause_ms_ + (now - start_time_ms_);
    if (total_elapsed >= timeout_ms_) {
        stop();
        has_error_ = true;
    }
}

void FillController::pause()
{
    if (!is_active_ || is_paused_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    elapsed_before_pause_ms_ += (now - start_time_ms_);

    valve_main_.turn_off();
    valve_softener_.turn_off();
    is_paused_ = true;
}

void FillController::resume()
{
    if (!is_active_ || !is_paused_) {
        return;
    }

    start_time_ms_ = timer_hal_.get_time_ms();
    is_paused_ = false;

    valve_main_.turn_on();
    if (use_softener_) {
        valve_softener_.turn_on();
    }
}

void FillController::stop()
{
    valve_main_.turn_off();
    valve_softener_.turn_off();
    is_active_ = false;
    is_paused_ = false;
}

} // namespace controllers
