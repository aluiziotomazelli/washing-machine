#pragma once

#include <gmock/gmock.h>
#include "ui/interfaces/i_buzzer.hpp"

namespace mocks {

class MockBuzzer : public ui::IBuzzer {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(void, beep, (uint16_t duration_ms), (override));
    MOCK_METHOD(void, play_pattern, (ui::BuzzerPattern pattern), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(bool, is_playing, (), (const, override));
};

} // namespace mocks
