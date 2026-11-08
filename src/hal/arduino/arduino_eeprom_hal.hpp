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

    /** @copydoc IEepromHAL::read_byte */
    uint8_t read_byte(uint16_t address) override;
    /** @copydoc IEepromHAL::write_byte */
    void write_byte(uint16_t address, uint8_t value) override;
    /** @copydoc IEepromHAL::read_block */
    void read_block(uint16_t address, void* dest, size_t length) override;
    /** @copydoc IEepromHAL::write_block */
    void write_block(uint16_t address, const void* src, size_t length) override;
    /** @copydoc IEepromHAL::capacity */
    size_t capacity() const override;
};

} // namespace hal
