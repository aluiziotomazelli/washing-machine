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
 * @brief Hardware Abstraction Layer interface for digital General Purpose I/O and tone generation.
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

    /**
     * @brief Generate a hardware square wave / audio tone on a pin (non-blocking).
     * @param pin Hardware pin number.
     * @param frequency_hz Tone frequency in Hertz.
     */
    virtual void play_tone(uint8_t pin, uint16_t frequency_hz) = 0;

    /**
     * @brief Stop hardware audio tone generation on a pin.
     * @param pin Hardware pin number.
     */
    virtual void stop_tone(uint8_t pin) = 0;
};

} // namespace hal
