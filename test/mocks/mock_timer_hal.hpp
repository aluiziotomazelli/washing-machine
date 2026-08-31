#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_timer_hal.hpp"

namespace mocks {

/**
 * @class MockTimerHAL
 * @brief Google Mock implementation of the ITimerHAL interface.
 */
class MockTimerHAL : public hal::ITimerHAL {
public:
    MOCK_METHOD(uint32_t, get_time_ms, (), (const, override));
    MOCK_METHOD(uint64_t, get_time_us, (), (const, override));
    MOCK_METHOD(void, delay_ms, (uint32_t ms), (override));
};

} // namespace mocks
