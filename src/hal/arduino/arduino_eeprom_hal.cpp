#include "arduino_eeprom_hal.hpp"
#include <EEPROM.h>

namespace hal {

uint8_t ArduinoEepromHAL::read_byte(uint16_t address)
{
    if (address >= capacity()) {
        return 0xFF;
    }
    return EEPROM.read(address);
}

void ArduinoEepromHAL::write_byte(uint16_t address, uint8_t value)
{
    if (address < capacity()) {
        EEPROM.update(address, value);
    }
}

void ArduinoEepromHAL::read_block(uint16_t address, void* dest, size_t length)
{
    if (dest == nullptr || address + length > capacity()) {
        return;
    }

    uint8_t* p = static_cast<uint8_t*>(dest);
    for (size_t i = 0; i < length; ++i) {
        p[i] = EEPROM.read(address + i);
    }
}

void ArduinoEepromHAL::write_block(uint16_t address, const void* src, size_t length)
{
    if (src == nullptr || address + length > capacity()) {
        return;
    }

    const uint8_t* p = static_cast<const uint8_t*>(src);
    for (size_t i = 0; i < length; ++i) {
        EEPROM.update(address + i, p[i]);
    }
}

size_t ArduinoEepromHAL::capacity() const
{
    return EEPROM.length();
}

} // namespace hal
