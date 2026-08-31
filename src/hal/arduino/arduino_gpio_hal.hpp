#pragma once

#include "../interfaces/i_gpio_hal.hpp"

namespace hal {

/**
 * @class ArduinoGpioHAL
 * @brief Concrete GPIO implementation using Arduino AVR core functions (pinMode, digitalWrite, digitalRead).
 */
class ArduinoGpioHAL : public IGpioHAL {
public:
    ArduinoGpioHAL() = default;
    ~ArduinoGpioHAL() override = default;

    void set_mode(uint8_t pin, GpioMode mode) override;
    void set_level(uint8_t pin, GpioLevel level) override;
    GpioLevel get_level(uint8_t pin) override;
};

} // namespace hal
