#pragma once

#include "../interfaces/i_eeprom_hal.hpp"

namespace hal {

/**
 * @class ArduinoEepromHAL
 * @brief Concrete EEPROM implementation using AVR EEPROM library (<EEPROM.h>).
 * Employs hardware-level update semantics to avoid unnecessary cell wear.
 */
class ArduinoEepromHAL : public IEepromHAL {
public:
    ArduinoEepromHAL() = default;
    ~ArduinoEepromHAL() override = default;

    uint8_t read_byte(uint16_t address) override;
    void write_byte(uint16_t address, uint8_t value) override;
    void read_block(uint16_t address, void* dest, size_t length) override;
    void write_block(uint16_t address, const void* src, size_t length) override;
    size_t capacity() const override;
};

} // namespace hal
