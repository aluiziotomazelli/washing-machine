#pragma once

#include "../interfaces/i_i2c_hal.hpp"

namespace hal {

/**
 * @class ArduinoI2cHAL
 * @brief Concrete I2C implementation wrapping Arduino's Wire library with anti-lockup timeout.
 */
class ArduinoI2cHAL : public II2cHAL {
public:
    ArduinoI2cHAL() = default;
    ~ArduinoI2cHAL() override = default;

    void init(uint32_t clock_hz = 100000) override;
    bool write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value) override;
    bool read_bytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t* buffer, size_t len) override;
};

} // namespace hal
