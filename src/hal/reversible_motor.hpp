#pragma once

#include <stdint.h>
#include "interfaces/i_reversible_motor.hpp"
#include "interfaces/i_gpio_hal.hpp"
#include "interfaces/i_timer_hal.hpp"

namespace hal {

/**
 * @class ReversibleMotor
 * @brief Platform-agnostic reversible AC motor driver with hardware/software mutual exclusion
 *        and non-blocking dead-time delay management.
 */
class ReversibleMotor : public IReversibleMotor {
public:
    /**
     * @brief Construct a reversible motor driver bound to a pair of direction pins.
     * @param gpio_hal GPIO abstraction used to drive the direction pins.
     * @param timer_hal Timer abstraction used to measure the dead-time window.
     * @param pin_clockwise GPIO pin that energizes the clockwise winding.
     * @param pin_counter_clockwise GPIO pin that energizes the counter-clockwise winding.
     * @param dead_time_ms Minimum idle time between direction reversals (default 200 ms).
     * @param active_low true when the relay/TRIAC driver is active on a LOW level.
     */
    ReversibleMotor(
        IGpioHAL& gpio_hal,
        ITimerHAL& timer_hal,
        uint8_t pin_clockwise,
        uint8_t pin_counter_clockwise,
        uint16_t dead_time_ms = 200,
        bool active_low = false
    );

    ~ReversibleMotor() override = default;

    /** @copydoc IReversibleMotor::init */
    void init() override;
    /** @copydoc IReversibleMotor::update */
    void update() override;
    /** @copydoc IReversibleMotor::rotate_clockwise */
    void rotate_clockwise() override;
    /** @copydoc IReversibleMotor::rotate_counter_clockwise */
    void rotate_counter_clockwise() override;
    /** @copydoc IReversibleMotor::stop */
    void stop() override;
    /** @copydoc IReversibleMotor::get_state */
    MotorState get_state() const override;

private:
    IGpioHAL& gpio_hal_;
    ITimerHAL& timer_hal_;
    uint8_t pin_cw_;
    uint8_t pin_ccw_;
    uint16_t dead_time_ms_;
    bool active_low_;

    MotorState current_state_{MotorState::STOPPED};
    MotorState pending_direction_{MotorState::STOPPED};
    uint32_t dead_time_start_ms_{0};
    bool is_initialized_{false};

    void apply_cw(bool on);
    void apply_ccw(bool on);
    void de_energize_all();
};

} // namespace hal
