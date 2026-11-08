#pragma once

#include "../interfaces/i_gpio_hal.hpp"

namespace hal {

/**
 * @class ArduinoGpioHAL
 * @brief Concrete GPIO and Tone implementation using standard Arduino core functions.
 */
class ArduinoGpioHAL : public IGpioHAL {
public:
    ArduinoGpioHAL() = default;
    ~ArduinoGpioHAL() override = default;

    /** @copydoc IGpioHAL::set_mode */
    void set_mode(uint8_t pin, GpioMode mode) override;
    /** @copydoc IGpioHAL::set_level */
    void set_level(uint8_t pin, GpioLevel level) override;
    /** @copydoc IGpioHAL::get_level */
    GpioLevel get_level(uint8_t pin) override;

    /** @copydoc IGpioHAL::play_tone */
    void play_tone(uint8_t pin, uint16_t frequency_hz) override;
    /** @copydoc IGpioHAL::stop_tone */
    void stop_tone(uint8_t pin) override;
};

} // namespace hal
