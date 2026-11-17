#pragma once

#include <stdint.h>

namespace controllers {

/**
 * @class AccelTimeline
 * @brief Pure non-blocking timer accounting for motor-on acceleration and wall-clock safety cap.
 *
 * Tracks accumulated motor-on duration during ACCEL phase, ensuring pause/damping intervals
 * do not count toward acceleration while enforcing a maximum wall-clock safety cap.
 * Zero HAL dependencies.
 *
 * Lifecycle/contract:
 * - Call start(now_ms, ...) to begin an active acceleration session.
 * - After start(), and whenever the motor is energized (ACCEL entry or resume from damping),
 *   call on_motor_on(now_ms) immediately. While the motor remains energized, call
 *   on_motor_on(now_ms) periodically (e.g. each update tick) to accumulate motor-on time.
 * - Call on_motor_off(now_ms) when the motor is cut/paused (damping/coast).
 * - Call reset() to end the session; an inactive/reset timeline is never complete.
 *
 * Timebase assumption:
 * - Requires monotonic, non-decreasing now_ms timestamps for the lifetime of the timeline
 *   (e.g. provided by ITimerHAL). uint32_t millisecond wraparound is not handled; acceptable
 *   for sessions on equipment that is normally powered off between uses.
 */
class AccelTimeline
{
public:
    /**
     * @brief Start or reset the acceleration timeline.
     * @param now_ms Current timestamp in milliseconds.
     * @param motor_on_target_ms Accumulated motor-on duration required to finish ACCEL.
     * @param wall_cap_ms Maximum wall-clock time allowed before forcing completion.
     */
    void start(uint32_t now_ms, uint32_t motor_on_target_ms = 15000, uint32_t wall_cap_ms = 40000);

    /**
     * @brief Notify timeline that motor is energized (CW acceleration).
     * @param now_ms Current timestamp in milliseconds.
     */
    void on_motor_on(uint32_t now_ms);

    /**
     * @brief Notify timeline that motor has been cut/paused (damping or coasting).
     * @param now_ms Current timestamp in milliseconds.
     */
    void on_motor_off(uint32_t now_ms);

    /**
     * @brief Check whether ACCEL phase is complete (either motor-on target or wall cap reached).
     * @param now_ms Current timestamp in milliseconds.
     * @return true when acceleration phase is complete for an active session.
     */
    bool is_complete(uint32_t now_ms) const;

    /**
     * @brief Get total accumulated motor-on time in milliseconds.
     * @return Duration in milliseconds for the current/last session state.
     */
    uint32_t motor_on_elapsed_ms() const { return motor_on_elapsed_ms_; }

    /**
     * @brief Check whether motor is currently tracked as ON.
     * @return true while motor is energized.
     */
    bool is_motor_on() const { return motor_on_; }

    /**
     * @brief Check whether the timeline has an active session.
     * @return true after start() and before reset().
     */
    bool is_active() const { return active_; }

    /**
     * @brief Reset timeline to inactive state.
     */
    void reset();

private:
    uint32_t start_ms_{0};
    uint32_t last_tick_ms_{0};
    uint32_t motor_on_elapsed_ms_{0};
    uint32_t motor_on_target_ms_{15000};
    uint32_t wall_cap_ms_{40000};
    bool motor_on_{false};
    bool active_{false};
};

} // namespace controllers
