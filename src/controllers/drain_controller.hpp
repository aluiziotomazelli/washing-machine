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
    /**
     * @brief Construct a drain controller bound to the drain pump, level sensor and timer.
     * @param timer_hal Timer abstraction used for timeout and bleed accounting.
     * @param drain_pump Drain pump output.
     * @param water_sensor Water level sensor used to detect an empty tub.
     * @param timeout_ms Maximum drain duration before raising an error (default 6 min).
     */
    DrainController(
        hal::ITimerHAL& timer_hal,
        hal::IDigitalOutput& drain_pump,
        hal::IWaterLevelSensor& water_sensor,
        uint32_t timeout_ms = 360000 // 6 minutes default timeout
    );

    /**
     * @brief Start tub drainage.
     * @param bleed_duration_ms Additional pumping time after tub is detected empty (default 20s).
     * @param already_empty_duration_ms Pumping time if tub is already empty at start (default 0s).
     */
    void start(uint32_t bleed_duration_ms = 20000, uint32_t already_empty_duration_ms = 0);

    /**
     * @brief Periodic non-blocking update (empty detection, bleed and timeout enforcement).
     */
    void update();

    /**
     * @brief Suspend drainage and stop the pump while preserving progress.
     */
    void pause();

    /**
     * @brief Resume a previously paused drainage operation.
     */
    void resume();

    /**
     * @brief Abort drainage immediately and stop the pump.
     */
    void stop();

    /**
     * @brief Release the drain output without stopping it (handover to another controller).
     */
    void handover() { is_active_ = false; is_paused_ = false; }

    /**
     * @brief Check whether a drain operation is running.
     * @return true while active.
     */
    bool is_active() const { return is_active_; }

    /**
     * @brief Check whether drainage is suspended.
     * @return true while paused.
     */
    bool is_paused() const { return is_paused_; }

    /**
     * @brief Check whether the post-empty bleed phase is running.
     * @return true while bleeding residual water.
     */
    bool is_bleeding() const { return bleeding_phase_; }

    /**
     * @brief Check whether drainage completed.
     * @return true when the operation has finished.
     */
    bool is_finished() const { return is_finished_; }

    /**
     * @brief Check whether a drain timeout occurred.
     * @return true when the timeout latched an error.
     */
    bool has_error() const { return has_error_; }

    /**
     * @brief Clear a latched drain timeout error.
     */
    void reset_error() { has_error_ = false; }

private:
    hal::ITimerHAL& timer_hal_;
    hal::IDigitalOutput& drain_pump_;
    hal::IWaterLevelSensor& water_sensor_;
    uint32_t timeout_ms_;

    uint32_t bleed_duration_ms_{20000};
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
