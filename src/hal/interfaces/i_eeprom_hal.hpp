#pragma once

#include <stdint.h>
#include <stddef.h>

namespace hal {

/**
 * @interface IEepromHAL
 * @brief Hardware Abstraction Layer interface for non-volatile EEPROM storage.
 */
class IEepromHAL {
public:
    virtual ~IEepromHAL() = default;

    /**
     * @brief Read a single byte from the specified EEPROM address.
     * @param address Zero-based byte offset into EEPROM.
     * @return Byte stored at the given address.
     */
    virtual uint8_t read_byte(uint16_t address) = 0;

    /**
     * @brief Write a single byte to the specified EEPROM address.
     * Implementations must use update semantics (only write physical cell if value changed)
     * to preserve EEPROM write cycle endurance.
     * @param address Zero-based byte offset into EEPROM.
     * @param value Byte value to write.
     */
    virtual void write_byte(uint16_t address, uint8_t value) = 0;

    /**
     * @brief Read a contiguous block of bytes from EEPROM.
     * @param address Starting address in EEPROM.
     * @param dest Pointer to destination memory buffer in RAM.
     * @param length Number of bytes to read.
     */
    virtual void read_block(uint16_t address, void* dest, size_t length) = 0;

    /**
     * @brief Write a contiguous block of bytes to EEPROM with cell-level update semantics.
     * @param address Starting address in EEPROM.
     * @param src Pointer to source memory buffer in RAM.
     * @param length Number of bytes to write.
     */
    virtual void write_block(uint16_t address, const void* src, size_t length) = 0;

    /**
     * @brief Get the total usable EEPROM storage capacity in bytes (e.g. 1024 on ATmega328P).
     * @return Total size of EEPROM memory space in bytes.
     */
    virtual size_t capacity() const = 0;
};

} // namespace hal
