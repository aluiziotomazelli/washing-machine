#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ui/panel_controller.hpp"
#include "mocks/mock_button.hpp"
#include "mocks/mock_buzzer.hpp"
#include "mocks/mock_led_panel.hpp"
#include "mocks/mock_timer_hal.hpp"
#include "mocks/mock_digital_output.hpp"
#include "mocks/mock_reversible_motor.hpp"
#include "mocks/mock_water_level_sensor.hpp"
#include "controllers/fill_controller.hpp"
#include "controllers/agitator.hpp"
#include "controllers/drain_controller.hpp"
#include "controllers/spin_controller.hpp"
#include "fsm/wash_cycle_coordinator.hpp"

using namespace testing;
using namespace domain;
using namespace ui;

class PanelControllerTest : public Test {
protected:
    NiceMock<mocks::MockTimerHAL> mock_timer;
    NiceMock<mocks::MockDigitalOutput> mock_valve_main;
    NiceMock<mocks::MockDigitalOutput> mock_valve_softener;
    NiceMock<mocks::MockDigitalOutput> mock_drain_pump;
    NiceMock<mocks::MockReversibleMotor> mock_motor;
    NiceMock<mocks::MockWaterLevelSensor> mock_water_sensor;

    controllers::FillController fill_ctrl{mock_timer, mock_valve_main, mock_valve_softener, mock_water_sensor, 5000};
    controllers::Agitator agitator{mock_timer, mock_motor};
    controllers::DrainController drain_ctrl{mock_timer, mock_drain_pump, mock_water_sensor, 5000};
    controllers::SpinConfig spin_config{100, 100, 100, 200};
    controllers::SpinController spin_ctrl{mock_timer, mock_drain_pump, mock_motor, spin_config};

    fsm::WashCycleCoordinator coordinator{mock_timer, fill_ctrl, agitator, drain_ctrl, spin_ctrl};

    NiceMock<mocks::MockButton> btn_start;
    NiceMock<mocks::MockButton> btn_program;
    NiceMock<mocks::MockButton> btn_level;
    NiceMock<mocks::MockButton> btn_softener;
    NiceMock<mocks::MockLedPanel> led_panel;
    NiceMock<mocks::MockBuzzer> buzzer;

    PanelController panel_ctrl{
        btn_start,
        btn_program,
        btn_level,
        btn_softener,
        led_panel,
        buzzer,
        coordinator
    };

    void SetUp() override {
        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Return(1000));
        ON_CALL(btn_start, get_last_click()).WillByDefault(Return(ButtonClickType::NONE_CLICK));
        ON_CALL(btn_program, get_last_click()).WillByDefault(Return(ButtonClickType::NONE_CLICK));
        ON_CALL(btn_level, get_last_click()).WillByDefault(Return(ButtonClickType::NONE_CLICK));
        ON_CALL(btn_softener, get_last_click()).WillByDefault(Return(ButtonClickType::NONE_CLICK));
    }
};

TEST_F(PanelControllerTest, InitializesPanelStateAtBoot)
{
    EXPECT_CALL(led_panel, set_program(WashProgram::RINSE_ONLY)).Times(1);
    EXPECT_CALL(led_panel, set_selected_level(WaterLevel::LOW_LEVEL)).Times(1);
    EXPECT_CALL(led_panel, set_softener(false)).Times(1);
    EXPECT_CALL(led_panel, set_machine_state(MachineState::IDLE, MachineError::NONE)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);

    panel_ctrl.init();

    EXPECT_EQ(panel_ctrl.get_selected_program(), WashProgram::RINSE_ONLY);
    EXPECT_EQ(panel_ctrl.get_selected_level(), WaterLevel::LOW_LEVEL);
    EXPECT_FALSE(panel_ctrl.is_softener_enabled());
}

TEST_F(PanelControllerTest, CyclesProgramsInIdleMode)
{
    panel_ctrl.init();

    // 1st click: RINSE_ONLY -> SPIN_ONLY
    EXPECT_CALL(btn_program, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(led_panel, set_program(WashProgram::SPIN_ONLY)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    panel_ctrl.update();
    EXPECT_EQ(panel_ctrl.get_selected_program(), WashProgram::SPIN_ONLY);

    // 2nd click: SPIN_ONLY -> NORMAL_WASH
    EXPECT_CALL(btn_program, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(led_panel, set_program(WashProgram::NORMAL_WASH)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    panel_ctrl.update();
    EXPECT_EQ(panel_ctrl.get_selected_program(), WashProgram::NORMAL_WASH);

    // 3rd click: NORMAL_WASH -> HEAVY_WASH
    EXPECT_CALL(btn_program, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(led_panel, set_program(WashProgram::HEAVY_WASH)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    panel_ctrl.update();
    EXPECT_EQ(panel_ctrl.get_selected_program(), WashProgram::HEAVY_WASH);

    // 4th click: HEAVY_WASH -> RINSE_ONLY
    EXPECT_CALL(btn_program, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(led_panel, set_program(WashProgram::RINSE_ONLY)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    panel_ctrl.update();
    EXPECT_EQ(panel_ctrl.get_selected_program(), WashProgram::RINSE_ONLY);
}

TEST_F(PanelControllerTest, IgnoresProgramClickWhenRunning)
{
    panel_ctrl.init();
    coordinator.start_cycle(WashProgram::NORMAL_WASH, WaterLevel::LOW_LEVEL, false);

    EXPECT_CALL(btn_program, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(led_panel, set_program(_)).Times(0);
    EXPECT_CALL(buzzer, beep(_)).Times(0);

    panel_ctrl.update();
    EXPECT_EQ(panel_ctrl.get_selected_program(), WashProgram::RINSE_ONLY);
}

TEST_F(PanelControllerTest, TogglesWaterLevelInIdle)
{
    panel_ctrl.init();

    // 1st click: LOW_LEVEL -> MEDIUM_LEVEL
    EXPECT_CALL(btn_level, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(led_panel, set_selected_level(WaterLevel::MEDIUM_LEVEL)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    panel_ctrl.update();
    EXPECT_EQ(panel_ctrl.get_selected_level(), WaterLevel::MEDIUM_LEVEL);

    // 2nd click: MEDIUM_LEVEL -> LOW_LEVEL
    EXPECT_CALL(btn_level, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(led_panel, set_selected_level(WaterLevel::LOW_LEVEL)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    panel_ctrl.update();
    EXPECT_EQ(panel_ctrl.get_selected_level(), WaterLevel::LOW_LEVEL);
}

TEST_F(PanelControllerTest, TogglesSoftenerInIdle)
{
    panel_ctrl.init();

    // 1st click: false -> true
    EXPECT_CALL(btn_softener, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(led_panel, set_softener(true)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    panel_ctrl.update();
    EXPECT_TRUE(panel_ctrl.is_softener_enabled());

    // 2nd click: true -> false
    EXPECT_CALL(btn_softener, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(led_panel, set_softener(false)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    panel_ctrl.update();
    EXPECT_FALSE(panel_ctrl.is_softener_enabled());
}

TEST_F(PanelControllerTest, StartPauseClickStartsCycleWhenIdle)
{
    panel_ctrl.init();

    EXPECT_CALL(btn_start, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    EXPECT_CALL(led_panel, set_machine_state(MachineState::RUNNING, MachineError::NONE)).Times(1);
    EXPECT_CALL(led_panel, set_stage(WashStage::RINSE)).Times(1);

    panel_ctrl.update();
    EXPECT_EQ(coordinator.get_state(), MachineState::RUNNING);
}

TEST_F(PanelControllerTest, StartPauseClickPausesCycleWhenRunning)
{
    panel_ctrl.init();
    coordinator.start_cycle(WashProgram::NORMAL_WASH, WaterLevel::LOW_LEVEL, false);

    EXPECT_CALL(btn_start, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(buzzer, play_pattern(BuzzerPattern::DOUBLE_BEEP)).Times(1);
    EXPECT_CALL(led_panel, set_machine_state(MachineState::PAUSED, MachineError::NONE)).Times(1);

    panel_ctrl.update();
    EXPECT_EQ(coordinator.get_state(), MachineState::PAUSED);
}

TEST_F(PanelControllerTest, StartPauseClickResumesCycleWhenPaused)
{
    panel_ctrl.init();
    coordinator.start_cycle(WashProgram::NORMAL_WASH, WaterLevel::LOW_LEVEL, false);
    coordinator.pause_cycle();

    EXPECT_CALL(btn_start, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(buzzer, beep(50)).Times(1);
    EXPECT_CALL(led_panel, set_machine_state(MachineState::RUNNING, MachineError::NONE)).Times(1);

    panel_ctrl.update();
    EXPECT_EQ(coordinator.get_state(), MachineState::RUNNING);
}

TEST_F(PanelControllerTest, StartPauseLongClickAdvancesStep)
{
    panel_ctrl.init();
    coordinator.start_cycle(WashProgram::NORMAL_WASH, WaterLevel::LOW_LEVEL, false);
    EXPECT_EQ(coordinator.get_step_index(), 0);

    EXPECT_CALL(btn_start, get_last_click()).WillOnce(Return(ButtonClickType::LONG_CLICK));
    EXPECT_CALL(buzzer, play_pattern(BuzzerPattern::LONG_BEEP)).Times(1);

    panel_ctrl.update();
    EXPECT_EQ(coordinator.get_step_index(), 1);
}

TEST_F(PanelControllerTest, StartPauseVeryLongClickStopsCycle)
{
    panel_ctrl.init();

    // Start cycle via panel click
    EXPECT_CALL(btn_start, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    panel_ctrl.update();
    EXPECT_EQ(coordinator.get_state(), MachineState::RUNNING);

    // Now very long click to cancel/stop
    EXPECT_CALL(btn_start, get_last_click()).WillOnce(Return(ButtonClickType::VERY_LONG_CLICK));
    EXPECT_CALL(buzzer, play_pattern(BuzzerPattern::DOUBLE_BEEP)).Times(1);
    EXPECT_CALL(led_panel, set_machine_state(MachineState::IDLE, MachineError::NONE)).Times(1);
    EXPECT_CALL(led_panel, set_stage(WashStage::IDLE)).Times(1);
    EXPECT_CALL(led_panel, set_program(WashProgram::RINSE_ONLY)).Times(1);

    panel_ctrl.update();
    EXPECT_EQ(coordinator.get_state(), MachineState::IDLE);
}

TEST_F(PanelControllerTest, SyncsFinishedStateAndPlaysTune)
{
    panel_ctrl.init();

    // Start cycle
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));
    panel_ctrl.update();
    EXPECT_EQ(coordinator.get_state(), MachineState::RUNNING);

    // Advance to completion (RINSE_ONLY recipe has 4 steps: 0, 1, 2, 3 -> FINISHED on 4)
    coordinator.advance_step(); // step 1
    coordinator.advance_step(); // step 2
    coordinator.advance_step(); // step 3
    coordinator.advance_step(); // step 4 -> FINISHED
    EXPECT_EQ(coordinator.get_state(), MachineState::FINISHED);

    EXPECT_CALL(buzzer, play_pattern(BuzzerPattern::CYCLE_FINISHED)).Times(1);
    EXPECT_CALL(led_panel, set_machine_state(MachineState::FINISHED, MachineError::NONE)).Times(1);
    EXPECT_CALL(led_panel, set_stage(WashStage::IDLE)).Times(1);
    EXPECT_CALL(led_panel, set_program(WashProgram::RINSE_ONLY)).Times(1);

    panel_ctrl.update();
}

TEST_F(PanelControllerTest, SyncsErrorStateAndPlaysAlarm)
{
    panel_ctrl.init();

    // Start cycle
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));
    panel_ctrl.update();

    // Force fill controller timeout
    ON_CALL(mock_timer, get_time_ms()).WillByDefault(Return(10000));
    coordinator.update();
    EXPECT_EQ(coordinator.get_state(), MachineState::ERROR);
    EXPECT_EQ(coordinator.get_error(), MachineError::FILL_TIMEOUT);

    EXPECT_CALL(buzzer, play_pattern(BuzzerPattern::ERROR_ALARM)).Times(1);
    EXPECT_CALL(led_panel, set_machine_state(MachineState::ERROR, MachineError::FILL_TIMEOUT)).Times(1);

    panel_ctrl.update();
}

TEST_F(PanelControllerTest, ButtonClickInFinishedStateWakesUpToIdle)
{
    panel_ctrl.init();

    // Start and advance to FINISHED
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));
    panel_ctrl.update();

    coordinator.advance_step();
    coordinator.advance_step();
    coordinator.advance_step();
    coordinator.advance_step();
    panel_ctrl.update();
    EXPECT_EQ(coordinator.get_state(), MachineState::FINISHED);

    // Clicking Water Level button while in FINISHED:
    // Should transition coordinator to IDLE, update machine state, set level, and beep!
    EXPECT_CALL(led_panel, set_machine_state(MachineState::IDLE, MachineError::NONE)).Times(AtLeast(1));
    EXPECT_CALL(led_panel, set_selected_level(WaterLevel::MEDIUM_LEVEL)).Times(1);
    EXPECT_CALL(buzzer, beep(50)).Times(1);

    EXPECT_CALL(btn_level, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    panel_ctrl.update();
    EXPECT_EQ(coordinator.get_state(), MachineState::IDLE);
}
