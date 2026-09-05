#include "arduino_i2c_hal.hpp"

#if defined(ARDUINO)
#include <Wire.h>
#endif

namespace hal {

void ArduinoI2cHAL::init(uint32_t clock_hz)
{
#if defined(ARDUINO)
    Wire.begin();
    Wire.setClock(clock_hz);
    // 25ms hardware timeout with bus auto-reset to prevent lockup in harsh electrical environments
    Wire.setWireTimeout(25000, true);
#else
    (void)clock_hz;
#endif
}

bool ArduinoI2cHAL::write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value)
{
#if defined(ARDUINO)
    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
#else
    (void)dev_addr;
    (void)reg_addr;
    (void)value;
    return false;
#endif
}

bool ArduinoI2cHAL::read_bytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t* buffer, size_t len)
{
#if defined(ARDUINO)
    if (buffer == nullptr || len == 0) {
        return false;
    }

    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    // Try repeated start first (sendStop = false); fallback to standard stop if rejected
    if (Wire.endTransmission(false) != 0) {
        Wire.beginTransmission(dev_addr);
        Wire.write(reg_addr);
        if (Wire.endTransmission(true) != 0) {
            return false;
        }
    }

    uint8_t received = Wire.requestFrom(dev_addr, static_cast<uint8_t>(len));
    if (received != len) {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        buffer[i] = static_cast<uint8_t>(Wire.read());
    }
    return true;
#else
    (void)dev_addr;
    (void)reg_addr;
    (void)buffer;
    (void)len;
    return false;
#endif
}

} // namespace hal
