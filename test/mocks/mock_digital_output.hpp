#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_digital_output.hpp"

namespace mocks {

class MockDigitalOutput : public hal::IDigitalOutput {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, turn_on, (), (override));
    MOCK_METHOD(void, turn_off, (), (override));
    MOCK_METHOD(void, toggle, (), (override));
    MOCK_METHOD(bool, is_on, (), (const, override));
    MOCK_METHOD(uint8_t, get_pin, (), (const, override));
};

} // namespace mocks
