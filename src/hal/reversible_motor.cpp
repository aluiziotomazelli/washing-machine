#include "reversible_motor.hpp"

namespace hal {

ReversibleMotor::ReversibleMotor(
    IGpioHAL& gpio_hal,
    ITimerHAL& timer_hal,
    uint8_t pin_clockwise,
    uint8_t pin_counter_clockwise,
    uint16_t dead_time_ms,
    bool active_low
)
    : gpio_hal_(gpio_hal)
    , timer_hal_(timer_hal)
    , pin_cw_(pin_clockwise)
    , pin_ccw_(pin_counter_clockwise)
    , dead_time_ms_(dead_time_ms)
    , active_low_(active_low)
    , current_state_(MotorState::STOPPED)
    , pending_direction_(MotorState::STOPPED)
    , dead_time_start_ms_(0)
    , is_initialized_(false)
{
}

void ReversibleMotor::init()
{
    gpio_hal_.set_mode(pin_cw_, GpioMode::MODE_OUTPUT);
    gpio_hal_.set_mode(pin_ccw_, GpioMode::MODE_OUTPUT);
    de_energize_all();
    is_initialized_ = true;
}

void ReversibleMotor::rotate_clockwise()
{
    if (!is_initialized_) {
        return;
    }

    if (current_state_ == MotorState::RUNNING_CLOCKWISE) {
        return;
    }

    if (current_state_ == MotorState::STOPPED) {
        apply_ccw(false);
        apply_cw(true);
        current_state_ = MotorState::RUNNING_CLOCKWISE;
    } else {
        // Must enforce dead-time before reversing or changing state
        de_energize_all();
        dead_time_start_ms_ = timer_hal_.get_time_ms();
        pending_direction_ = MotorState::RUNNING_CLOCKWISE;
        current_state_ = MotorState::DEAD_TIME_WAIT;
    }
}

void ReversibleMotor::rotate_counter_clockwise()
{
    if (!is_initialized_) {
        return;
    }

    if (current_state_ == MotorState::RUNNING_COUNTER_CLOCKWISE) {
        return;
    }

    if (current_state_ == MotorState::STOPPED) {
        apply_cw(false);
        apply_ccw(true);
        current_state_ = MotorState::RUNNING_COUNTER_CLOCKWISE;
    } else {
        // Must enforce dead-time before reversing or changing state
        de_energize_all();
        dead_time_start_ms_ = timer_hal_.get_time_ms();
        pending_direction_ = MotorState::RUNNING_COUNTER_CLOCKWISE;
        current_state_ = MotorState::DEAD_TIME_WAIT;
    }
}

void ReversibleMotor::stop()
{
    de_energize_all();
    pending_direction_ = MotorState::STOPPED;
    current_state_ = MotorState::STOPPED;
}

void ReversibleMotor::update()
{
    if (!is_initialized_) {
        return;
    }

    if (current_state_ == MotorState::DEAD_TIME_WAIT) {
        uint32_t now = timer_hal_.get_time_ms();
        if (now - dead_time_start_ms_ >= dead_time_ms_) {
            if (pending_direction_ == MotorState::RUNNING_CLOCKWISE) {
                apply_ccw(false);
                apply_cw(true);
                current_state_ = MotorState::RUNNING_CLOCKWISE;
            } else if (pending_direction_ == MotorState::RUNNING_COUNTER_CLOCKWISE) {
                apply_cw(false);
                apply_ccw(true);
                current_state_ = MotorState::RUNNING_COUNTER_CLOCKWISE;
            } else {
                current_state_ = MotorState::STOPPED;
            }
            pending_direction_ = MotorState::STOPPED;
        }
    }
}

MotorState ReversibleMotor::get_state() const
{
    return current_state_;
}

void ReversibleMotor::apply_cw(bool on)
{
    if (on) {
        // Mutual exclusion hardware guarantee
        gpio_hal_.set_level(pin_ccw_, active_low_ ? GpioLevel::LEVEL_HIGH : GpioLevel::LEVEL_LOW);
        gpio_hal_.set_level(pin_cw_, active_low_ ? GpioLevel::LEVEL_LOW : GpioLevel::LEVEL_HIGH);
    } else {
        gpio_hal_.set_level(pin_cw_, active_low_ ? GpioLevel::LEVEL_HIGH : GpioLevel::LEVEL_LOW);
    }
}

void ReversibleMotor::apply_ccw(bool on)
{
    if (on) {
        // Mutual exclusion hardware guarantee
        gpio_hal_.set_level(pin_cw_, active_low_ ? GpioLevel::LEVEL_HIGH : GpioLevel::LEVEL_LOW);
        gpio_hal_.set_level(pin_ccw_, active_low_ ? GpioLevel::LEVEL_LOW : GpioLevel::LEVEL_HIGH);
    } else {
        gpio_hal_.set_level(pin_ccw_, active_low_ ? GpioLevel::LEVEL_HIGH : GpioLevel::LEVEL_LOW);
    }
}

void ReversibleMotor::de_energize_all()
{
    gpio_hal_.set_level(pin_cw_, active_low_ ? GpioLevel::LEVEL_HIGH : GpioLevel::LEVEL_LOW);
    gpio_hal_.set_level(pin_ccw_, active_low_ ? GpioLevel::LEVEL_HIGH : GpioLevel::LEVEL_LOW);
}

} // namespace hal
