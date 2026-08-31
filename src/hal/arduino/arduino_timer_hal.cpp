#include "arduino_timer_hal.hpp"
#include <Arduino.h>

namespace hal {

uint32_t ArduinoTimerHAL::get_time_ms() const
{
    return millis();
}

uint64_t ArduinoTimerHAL::get_time_us() const
{
    return micros();
}

void ArduinoTimerHAL::delay_ms(uint32_t ms)
{
    delay(ms);
}

} // namespace hal
