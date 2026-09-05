#pragma once

#include <gmock/gmock.h>
#include "controllers/interfaces/i_vibration_monitor.hpp"

namespace mocks {

class MockVibrationMonitor : public controllers::IVibrationMonitor {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(uint16_t, get_vibration, (), (const, override));
    MOCK_METHOD((const hal::Vector3&), get_last_sample, (), (const, override));
    MOCK_METHOD(bool, is_in_motion, (), (const, override));
    MOCK_METHOD(bool, is_warning, (), (const, override));
    MOCK_METHOD(bool, is_critical_unbalance, (), (const, override));
    MOCK_METHOD(bool, is_sensor_ok, (), (const, override));
    MOCK_METHOD(void, reset, (), (override));
};

} // namespace mocks
