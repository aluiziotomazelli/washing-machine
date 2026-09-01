#pragma once

#include <stdint.h>

namespace hal {

/**
 * @interface IDigitalOutput
 * @brief Abstract interface for binary digital outputs (relays, solenoid valves, pumps, buzzers, LEDs).
 */
class IDigitalOutput {
public:
    virtual ~IDigitalOutput() = default;

    /**
     * @brief Initialize hardware pin direction and default state.
     */
    virtual void init() = 0;

    /**
     * @brief Energize / activate the output device.
     */
    virtual void turn_on() = 0;

    /**
     * @brief De-energize / deactivate the output device.
     */
    virtual void turn_off() = 0;

    /**
     * @brief Invert the current output state.
     */
    virtual void toggle() = 0;

    /**
     * @brief Query whether the device is currently active.
     * @return true if energized/active, false otherwise.
     */
    virtual bool is_on() const = 0;

    /**
     * @brief Get the hardware pin number assigned to this output.
     */
    virtual uint8_t get_pin() const = 0;
};

} // namespace hal
