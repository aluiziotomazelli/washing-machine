#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_watchdog_hal.hpp"

namespace mocks {

class MockWatchdogHAL : public hal::IWatchdogHAL {
public:
    MOCK_METHOD(void, enable, (hal::WatchdogTimeout timeout), (override));
    MOCK_METHOD(void, kick, (), (override));
    MOCK_METHOD(void, disable, (), (override));
    MOCK_METHOD(bool, was_reset_by_watchdog, (), (const, override));
};

} // namespace mocks
