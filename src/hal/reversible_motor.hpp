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
    ReversibleMotor(
        IGpioHAL& gpio_hal,
        ITimerHAL& timer_hal,
        uint8_t pin_clockwise,
        uint8_t pin_counter_clockwise,
        uint16_t dead_time_ms = 200,
        bool active_low = false
    );

    ~ReversibleMotor() override = default;

    void init() override;
    void update() override;
    void rotate_clockwise() override;
    void rotate_counter_clockwise() override;
    void stop() override;
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
