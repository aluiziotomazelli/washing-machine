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
     * @return true when acceleration phase is complete.
     */
    bool is_complete(uint32_t now_ms) const;

    /**
     * @brief Get total accumulated motor-on time in milliseconds.
     * @return Duration in milliseconds.
     */
    uint32_t motor_on_elapsed_ms() const { return motor_on_elapsed_ms_; }

    /**
     * @brief Check whether motor is currently tracked as ON.
     * @return true while motor is energized.
     */
    bool is_motor_on() const { return motor_on_; }

    /**
     * @brief Reset timeline to uninitialized state.
     */
    void reset();

private:
    uint32_t start_ms_{0};
    uint32_t last_tick_ms_{0};
    uint32_t motor_on_elapsed_ms_{0};
    uint32_t motor_on_target_ms_{15000};
    uint32_t wall_cap_ms_{40000};
    bool motor_on_{false};
};

} // namespace controllers
