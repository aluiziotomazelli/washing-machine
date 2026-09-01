#include "arduino_gpio_hal.hpp"
#include <Arduino.h>

namespace hal {

void ArduinoGpioHAL::set_mode(uint8_t pin, GpioMode mode)
{
    switch (mode) {
    case GpioMode::MODE_INPUT_PULLUP:
        pinMode(pin, INPUT_PULLUP);
        break;
    case GpioMode::MODE_OUTPUT:
        pinMode(pin, OUTPUT);
        break;
    case GpioMode::MODE_INPUT:
    default:
        pinMode(pin, INPUT);
        break;
    }
}

void ArduinoGpioHAL::set_level(uint8_t pin, GpioLevel level)
{
    digitalWrite(pin, (level == GpioLevel::LEVEL_HIGH) ? HIGH : LOW);
}

GpioLevel ArduinoGpioHAL::get_level(uint8_t pin)
{
    return (digitalRead(pin) == HIGH) ? GpioLevel::LEVEL_HIGH : GpioLevel::LEVEL_LOW;
}

void ArduinoGpioHAL::play_tone(uint8_t pin, uint16_t frequency_hz)
{
    tone(pin, frequency_hz);
}

void ArduinoGpioHAL::stop_tone(uint8_t pin)
{
    noTone(pin);
}

} // namespace hal
