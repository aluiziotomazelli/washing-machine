#pragma once

#include <stdint.h>

namespace hal {

/**
 * @brief Operating states of a reversible AC motor.
 */
enum class MotorState : uint8_t {
    STOPPED,
    RUNNING_CLOCKWISE,
    RUNNING_COUNTER_CLOCKWISE,
    DEAD_TIME_WAIT
};

/**
 * @interface IReversibleMotor
 * @brief Abstract interface for reversible AC induction motor control with safety interlocks.
 */
class IReversibleMotor {
public:
    virtual ~IReversibleMotor() = default;

    /**
     * @brief Initialize hardware pin modes and set safe initial state (stopped).
     */
    virtual void init() = 0;

    /**
     * @brief Non-blocking state update method (to be called periodically in loop).
     */
    virtual void update() = 0;

    /**
     * @brief Command the motor to rotate clockwise (Right).
     */
    virtual void rotate_clockwise() = 0;

    /**
     * @brief Command the motor to rotate counter-clockwise (Left).
     */
    virtual void rotate_counter_clockwise() = 0;

    /**
     * @brief Immediately de-energize both windings and stop the motor.
     */
    virtual void stop() = 0;

    /**
     * @brief Query current operating state of the motor.
     */
    virtual MotorState get_state() const = 0;
};

} // namespace hal
