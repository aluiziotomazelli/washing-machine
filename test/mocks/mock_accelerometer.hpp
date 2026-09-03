#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_accelerometer.hpp"

namespace mocks {

class MockAccelerometer : public hal::IAccelerometer {
public:
    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(bool, read_accel, (hal::Vector3&), (override));
};

} // namespace mocks
