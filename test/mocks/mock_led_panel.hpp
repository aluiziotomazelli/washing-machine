#pragma once

#include <gmock/gmock.h>
#include "ui/interfaces/i_led_panel.hpp"

namespace mocks {

class MockLedPanel : public ui::ILedPanel {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(void, set_machine_state, (domain::MachineState state), (override));
    MOCK_METHOD(void, set_softener, (bool enabled), (override));
    MOCK_METHOD(void, set_program, (domain::WashProgram program), (override));
    MOCK_METHOD(void, set_stage, (domain::WashStage stage), (override));
    MOCK_METHOD(void, set_selected_level, (domain::WaterLevel level), (override));
    MOCK_METHOD(void, turn_off_all, (), (override));
};

} // namespace mocks
