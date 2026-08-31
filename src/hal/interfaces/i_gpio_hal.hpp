#pragma once

#include <stdint.h>

namespace hal {

/**
 * @brief Pin direction and pull configuration.
 */
enum class GpioMode : uint8_t {
    MODE_INPUT,
    MODE_INPUT_PULLUP,
    MODE_OUTPUT
};

/**
 * @brief Logic level for digital pins.
 */
enum class GpioLevel : uint8_t {
    LEVEL_LOW = 0,
    LEVEL_HIGH = 1
};

/**
 * @interface IGpioHAL
 * @brief Hardware Abstraction Layer interface for digital General Purpose I/O (Pins).
 */
class IGpioHAL {
public:
    virtual ~IGpioHAL() = default;

    /**
     * @brief Configure the operating mode of a pin.
     * @param pin Hardware pin number.
     * @param mode Desired mode (MODE_INPUT, MODE_INPUT_PULLUP, MODE_OUTPUT).
     */
    virtual void set_mode(uint8_t pin, GpioMode mode) = 0;

    /**
     * @brief Set digital output level of a pin.
     * @param pin Hardware pin number.
     * @param level Output level (LEVEL_LOW or LEVEL_HIGH).
     */
    virtual void set_level(uint8_t pin, GpioLevel level) = 0;

    /**
     * @brief Read the digital input level of a pin.
     * @param pin Hardware pin number.
     * @return Current logic level (LEVEL_LOW or LEVEL_HIGH).
     */
    virtual GpioLevel get_level(uint8_t pin) = 0;
};

} // namespace hal
