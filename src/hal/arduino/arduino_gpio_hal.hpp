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

    void set_mode(uint8_t pin, GpioMode mode) override;
    void set_level(uint8_t pin, GpioLevel level) override;
    GpioLevel get_level(uint8_t pin) override;

    void play_tone(uint8_t pin, uint16_t frequency_hz) override;
    void stop_tone(uint8_t pin) override;
};

} // namespace hal
