#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_gpio_hal.hpp"

namespace mocks {

/**
 * @class MockGpioHAL
 * @brief Google Mock implementation of the IGpioHAL interface.
 */
class MockGpioHAL : public hal::IGpioHAL {
public:
    MOCK_METHOD(void, set_mode, (uint8_t pin, hal::GpioMode mode), (override));
    MOCK_METHOD(void, set_level, (uint8_t pin, hal::GpioLevel level), (override));
    MOCK_METHOD(hal::GpioLevel, get_level, (uint8_t pin), (override));
};

} // namespace mocks
