#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_i2c_hal.hpp"

namespace mocks {

class MockI2cHAL : public hal::II2cHAL {
public:
    MOCK_METHOD(void, init, (uint32_t clock_hz), (override));
    MOCK_METHOD(bool, write_reg, (uint8_t dev_addr, uint8_t reg_addr, uint8_t value), (override));
    MOCK_METHOD(bool, read_bytes, (uint8_t dev_addr, uint8_t reg_addr, uint8_t* buffer, size_t len), (override));
};

} // namespace mocks
