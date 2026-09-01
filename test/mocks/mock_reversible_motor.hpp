#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_reversible_motor.hpp"

namespace mocks {

class MockReversibleMotor : public hal::IReversibleMotor {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, rotate_clockwise, (), (override));
    MOCK_METHOD(void, rotate_counter_clockwise, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(hal::MotorState, get_state, (), (const, override));
};

} // namespace mocks
