#pragma once

#include <stdint.h>
#include "../hal/interfaces/i_reversible_motor.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace controllers {

/**
 * @class Agitator
 * @brief Manages mechanical washing agitation, alternating CW/CCW motor rotation with precise on/off intervals.
 */
class Agitator {
public:
    /**
     * @brief Construct an agitator bound to a reversible motor and timer.
     * @param timer_hal Timer abstraction used for stroke and duration timing.
     * @param motor Reversible motor driven by the agitator.
     */
    Agitator(hal::ITimerHAL& timer_hal, hal::IReversibleMotor& motor);

    /**
     * @brief Start an agitation cycle.
     * @param duration_sec Total duration of the agitation phase in seconds.
     * @param on_ms Pulse duration in milliseconds (e.g. 300 ms).
     * @param off_ms Pause duration between strokes in milliseconds (e.g. 200 ms or 300 ms).
     */
    void start(uint32_t duration_sec, uint16_t on_ms = 300, uint16_t off_ms = 200);

    /**
     * @brief Periodic non-blocking update (stroke alternation and duration tracking).
     */
    void update();

    /**
     * @brief Suspend agitation and stop the motor.
     */
    void pause();

    /**
     * @brief Resume a previously paused agitation cycle.
     */
    void resume();

    /**
     * @brief Stop agitation and de-energize the motor.
     */
    void stop();

    /**
     * @brief Check whether an agitation cycle is running.
     * @return true while active.
     */
    bool is_active() const { return is_active_; }

    /**
     * @brief Check whether agitation is suspended.
     * @return true while paused.
     */
    bool is_paused() const { return is_paused_; }

    /**
     * @brief Check whether the agitation duration elapsed.
     * @return true when the cycle has finished.
     */
    bool is_finished() const { return is_finished_; }

private:
    hal::ITimerHAL& timer_hal_;
    hal::IReversibleMotor& motor_;

    uint32_t duration_ms_{0};
    uint16_t on_ms_{300};
    uint16_t off_ms_{200};

    uint32_t step_start_ms_{0};
    uint32_t elapsed_before_pause_ms_{0};
    uint32_t pause_start_ms_{0};

    uint32_t stroke_start_ms_{0};
    bool is_cw_stroke_{true};
    bool stroke_motor_on_{false};

    bool is_active_{false};
    bool is_paused_{false};
    bool is_finished_{false};
};

} // namespace controllers
