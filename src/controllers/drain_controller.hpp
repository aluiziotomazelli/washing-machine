#pragma once

#include <stdint.h>
#include "../hal/interfaces/i_digital_output.hpp"
#include "../hal/interfaces/i_water_level_sensor.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace controllers {

/**
 * @class DrainController
 * @brief Manages tub drainage, controlling the drain pump, bleeding residual water, and enforcing timeout limits.
 * Supports pause() and resume() to suspend drainage without resetting progress or timeouts.
 */
class DrainController {
public:
    DrainController(
        hal::ITimerHAL& timer_hal,
        hal::IDigitalOutput& drain_pump,
        hal::IWaterLevelSensor& water_sensor,
        uint32_t timeout_ms = 360000 // 6 minutes default timeout
    );

    /**
     * @brief Start tub drainage.
     * @param bleed_duration_ms Additional pumping time after tub is detected empty (default 30s).
     * @param already_empty_duration_ms Pumping time if tub is already empty at start (default 0s).
     */
    void start(uint32_t bleed_duration_ms = 30000, uint32_t already_empty_duration_ms = 0);

    void update();
    void pause();
    void resume();
    void stop();
    void handover() { is_active_ = false; is_paused_ = false; }

    bool is_active() const { return is_active_; }
    bool is_paused() const { return is_paused_; }
    bool is_bleeding() const { return bleeding_phase_; }
    bool is_finished() const { return is_finished_; }
    bool has_error() const { return has_error_; }
    void reset_error() { has_error_ = false; }

private:
    hal::ITimerHAL& timer_hal_;
    hal::IDigitalOutput& drain_pump_;
    hal::IWaterLevelSensor& water_sensor_;
    uint32_t timeout_ms_;

    uint32_t bleed_duration_ms_{30000};
    uint32_t start_time_ms_{0};
    uint32_t bleed_start_ms_{0};

    uint32_t drain_elapsed_before_pause_ms_{0};
    uint32_t bleed_elapsed_before_pause_ms_{0};

    bool bleeding_phase_{false};
    bool is_active_{false};
    bool is_paused_{false};
    bool is_finished_{false};
    bool has_error_{false};
};

} // namespace controllers
