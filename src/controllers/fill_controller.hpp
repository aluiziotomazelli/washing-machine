#pragma once

#include <stdint.h>
#include "../domain/wash_types.hpp"
#include "../hal/interfaces/i_digital_output.hpp"
#include "../hal/interfaces/i_water_level_sensor.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace controllers {

/**
 * @class FillController
 * @brief Manages water filling sequence, controlling inlet valves and enforcing timeout protection.
 * Supports pause() and resume() to suspend filling and freeze timeout calculations.
 */
class FillController {
public:
    /**
     * @brief Construct a fill controller bound to valves, level sensor and timer.
     * @param timer_hal Timer abstraction used for timeout accounting.
     * @param valve_main Main inlet valve output.
     * @param valve_softener Softener dispenser valve output.
     * @param water_sensor Water level sensor used to detect target level.
     * @param timeout_ms Maximum fill duration before raising an error (default 20 min).
     */
    FillController(
        hal::ITimerHAL& timer_hal,
        hal::IDigitalOutput& valve_main,
        hal::IDigitalOutput& valve_softener,
        hal::IWaterLevelSensor& water_sensor,
        uint32_t timeout_ms = 20UL * 60 * 1000 // 20 minutes default
    );

    /**
     * @brief Start filling toward a target water level.
     * @param target Desired water level.
     * @param use_softener true to also open the softener valve.
     */
    void start(domain::WaterLevel target, bool use_softener = false);

    /**
     * @brief Periodic non-blocking update (level polling and timeout enforcement).
     */
    void update();

    /**
     * @brief Suspend filling and freeze the timeout accounting.
     */
    void pause();

    /**
     * @brief Resume a previously paused fill without resetting elapsed time.
     */
    void resume();

    /**
     * @brief Abort filling immediately and close the valves.
     */
    void stop();

    /**
     * @brief Check whether a fill operation is currently running.
     * @return true while filling is active.
     */
    bool is_active() const { return is_active_; }

    /**
     * @brief Check whether the fill is suspended.
     * @return true while paused.
     */
    bool is_paused() const { return is_paused_; }

    /**
     * @brief Check whether the fill reached its target or was stopped.
     * @return true when the operation has finished.
     */
    bool is_finished() const { return is_finished_; }

    /**
     * @brief Check whether a fill timeout occurred.
     * @return true when the timeout latched an error.
     */
    bool has_error() const { return has_error_; }

    /**
     * @brief Clear a latched fill timeout error.
     */
    void reset_error() { has_error_ = false; }

private:
    hal::ITimerHAL& timer_hal_;
    hal::IDigitalOutput& valve_main_;
    hal::IDigitalOutput& valve_softener_;
    hal::IWaterLevelSensor& water_sensor_;
    uint32_t timeout_ms_;

    domain::WaterLevel target_level_{domain::WaterLevel::LOW_LEVEL};
    bool use_softener_{false};
    uint32_t start_time_ms_{0};
    uint32_t elapsed_before_pause_ms_{0};

    bool is_active_{false};
    bool is_paused_{false};
    bool is_finished_{false};
    bool has_error_{false};
};

} // namespace controllers
