#pragma once

#include <stdint.h>
#include <stddef.h>

namespace hal {

/**
 * @interface II2cHAL
 * @brief Hardware Abstraction Layer interface for I2C (Two-Wire Interface) bus transactions.
 */
class II2cHAL {
public:
    virtual ~II2cHAL() = default;

    /**
     * @brief Initialize I2C peripheral and clock speed.
     * @param clock_hz Bus clock frequency in Hz (default: 100000 Hz / 100 kHz).
     */
    virtual void init(uint32_t clock_hz = 100000) = 0;

    /**
     * @brief Write a single byte to a device register.
     * @param dev_addr 7-bit I2C device address.
     * @param reg_addr Register address inside the device.
     * @param value Byte value to write.
     * @return true on ACK, false on NACK, bus error or timeout.
     */
    virtual bool write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value) = 0;

    /**
     * @brief Read contiguous bytes from a device register.
     * @param dev_addr 7-bit I2C device address.
     * @param reg_addr Register address to start reading from.
     * @param buffer Pointer to destination memory buffer.
     * @param len Number of bytes to read.
     * @return true on success with full byte count received, false on any communication fault.
     */
    virtual bool read_bytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t* buffer, size_t len) = 0;
};

} // namespace hal
