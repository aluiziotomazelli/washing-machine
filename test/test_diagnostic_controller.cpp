#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ui/diagnostic_controller.hpp"
#include "mocks/mock_button.hpp"
#include "mocks/mock_led_panel.hpp"
#include "mocks/mock_buzzer.hpp"
#include "mocks/mock_water_level_sensor.hpp"
#include "mocks/mock_vibration_monitor.hpp"
#include "mocks/mock_digital_output.hpp"
#include "mocks/mock_reversible_motor.hpp"
#include "mocks/mock_timer_hal.hpp"

using namespace ui;
using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

class DiagnosticControllerTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockButton> btn_start;
    NiceMock<mocks::MockButton> btn_program;
    NiceMock<mocks::MockLedPanel> led_panel;
    NiceMock<mocks::MockBuzzer> buzzer;
    NiceMock<mocks::MockWaterLevelSensor> water_sensor;
    NiceMock<mocks::MockVibrationMonitor> vib_monitor;
    NiceMock<mocks::MockDigitalOutput> valve_main;
    NiceMock<mocks::MockDigitalOutput> valve_softener;
    NiceMock<mocks::MockDigitalOutput> drain_pump;
    NiceMock<mocks::MockReversibleMotor> motor;
    NiceMock<mocks::MockTimerHAL> timer_hal;

    DiagnosticController diag_ctrl{
        btn_start,
        btn_program,
        led_panel,
        buzzer,
        water_sensor,
        valve_main,
        valve_softener,
        drain_pump,
        motor,
        timer_hal,
        &vib_monitor
    };

    bool pump_state{false};

    void SetUp() override {
        pump_state = false;
        ON_CALL(btn_start, get_last_click()).WillByDefault(Return(ButtonClickType::NONE_CLICK));
        ON_CALL(btn_program, get_last_click()).WillByDefault(Return(ButtonClickType::NONE_CLICK));
        ON_CALL(water_sensor, get_current_level()).WillByDefault(Return(domain::WaterLevel::LOW_LEVEL));
        ON_CALL(timer_hal, get_time_ms()).WillByDefault(Return(1000));
        ON_CALL(motor, get_state()).WillByDefault(Return(hal::MotorState::STOPPED));
        ON_CALL(vib_monitor, is_critical_unbalance()).WillByDefault(Return(false));
        ON_CALL(vib_monitor, get_vibration()).WillByDefault(Return(200));
        ON_CALL(vib_monitor, is_sensor_ok()).WillByDefault(Return(true));

        ON_CALL(drain_pump, turn_on()).WillByDefault([this]() { pump_state = true; });
        ON_CALL(drain_pump, turn_off()).WillByDefault([this]() { pump_state = false; });
        ON_CALL(drain_pump, is_on()).WillByDefault([this]() { return pump_state; });
    }
};

TEST_F(DiagnosticControllerTest, InitializesInInactiveState)
{
    EXPECT_FALSE(diag_ctrl.is_active());
}

TEST_F(DiagnosticControllerTest, EnterActivatesControllerAndSetsInitialStep)
{
    EXPECT_CALL(buzzer, play_pattern(BuzzerPattern::DOUBLE_BEEP)).Times(1);

    diag_ctrl.enter();

    EXPECT_TRUE(diag_ctrl.is_active());
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::LEVEL_SENSOR);
}

TEST_F(DiagnosticControllerTest, AdvancesStepsWithProgramButtonClick)
{
    diag_ctrl.enter();

    EXPECT_CALL(btn_program, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    EXPECT_CALL(buzzer, beep(50)).Times(7);

    // Step 0 -> Step 1 (VIBRATION_SENSOR)
    diag_ctrl.update();
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::VIBRATION_SENSOR);

    // Step 1 -> Step 2 (MAIN_VALVE)
    diag_ctrl.update();
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::MAIN_VALVE);

    // Step 2 -> Step 3 (SOFTENER_VALVE)
    diag_ctrl.update();
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::SOFTENER_VALVE);

    // Step 3 -> Step 4 (DRAIN_PUMP)
    diag_ctrl.update();
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::DRAIN_PUMP);

    // Step 4 -> Step 5 (MOTOR_AGITATE)
    diag_ctrl.update();
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::MOTOR_AGITATE);

    // Step 5 -> Step 6 (SPIN_TEST)
    diag_ctrl.update();
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::SPIN_TEST);

    // Step 6 -> Wraps to Step 0 (LEVEL_SENSOR)
    diag_ctrl.update();
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::LEVEL_SENSOR);
}

TEST_F(DiagnosticControllerTest, ExitsDiagnosticModeOnStartLongClick)
{
    diag_ctrl.enter();

    EXPECT_CALL(btn_start, get_last_click()).WillOnce(Return(ButtonClickType::LONG_CLICK));
    EXPECT_CALL(valve_main, turn_off()).Times(1);
    EXPECT_CALL(valve_softener, turn_off()).Times(1);
    EXPECT_CALL(drain_pump, turn_off()).Times(1);
    EXPECT_CALL(motor, stop()).Times(1);
    EXPECT_CALL(led_panel, turn_off_all()).Times(1);
    EXPECT_CALL(buzzer, play_pattern(BuzzerPattern::DOUBLE_BEEP)).Times(1);

    diag_ctrl.update();

    EXPECT_FALSE(diag_ctrl.is_active());
}

TEST_F(DiagnosticControllerTest, LevelSensorStepQueriesWaterSensorAndUpdatesPanel)
{
    diag_ctrl.enter();

    EXPECT_CALL(water_sensor, get_current_level()).WillOnce(Return(domain::WaterLevel::MEDIUM_LEVEL));
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::LEVEL_SENSOR, static_cast<uint16_t>(domain::WaterLevel::MEDIUM_LEVEL), true)).Times(1);

    diag_ctrl.update();
}

TEST_F(DiagnosticControllerTest, VibrationSensorStepQueriesVibrationMonitorAndUpdatesPanel)
{
    diag_ctrl.enter();

    EXPECT_CALL(btn_program, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(vib_monitor, update()).Times(1);
    EXPECT_CALL(vib_monitor, get_vibration()).WillOnce(Return(1250));
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::VIBRATION_SENSOR, 1250, true)).Times(1);

    diag_ctrl.update();
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::VIBRATION_SENSOR);
}

TEST_F(DiagnosticControllerTest, VibrationSensorStepShowsRedWhenSensorFails)
{
    diag_ctrl.enter();

    EXPECT_CALL(btn_program, get_last_click()).WillOnce(Return(ButtonClickType::CLICK));
    EXPECT_CALL(vib_monitor, update()).Times(1);
    EXPECT_CALL(vib_monitor, is_sensor_ok()).WillOnce(Return(false));
    EXPECT_CALL(vib_monitor, get_vibration()).WillOnce(Return(0));
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::VIBRATION_SENSOR, 0, false)).Times(1);

    diag_ctrl.update();
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::VIBRATION_SENSOR);
}

TEST_F(DiagnosticControllerTest, MainValveStepTogglesWithStartClick)
{
    diag_ctrl.enter();

    // Navigate to MAIN_VALVE (2 clicks on program)
    EXPECT_CALL(btn_program, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    diag_ctrl.update(); // Step 1: VIBRATION_SENSOR
    diag_ctrl.update(); // Step 2: MAIN_VALVE
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::MAIN_VALVE);

    // Initial state: valve OFF
    EXPECT_FALSE(diag_ctrl.is_valve_active());

    // Click Start -> valve turns ON
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    EXPECT_CALL(valve_main, turn_on()).Times(1);
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::MAIN_VALVE, 1, true)).Times(1);
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    diag_ctrl.update();
    EXPECT_TRUE(diag_ctrl.is_valve_active());

    // Click Start again -> valve turns OFF
    EXPECT_CALL(valve_main, turn_off()).Times(1);
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::MAIN_VALVE, 0, true)).Times(1);
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    diag_ctrl.update();
    EXPECT_FALSE(diag_ctrl.is_valve_active());
}

TEST_F(DiagnosticControllerTest, SoftenerValveStepTogglesWithStartClick)
{
    diag_ctrl.enter();

    // Navigate to SOFTENER_VALVE (3 clicks on program)
    EXPECT_CALL(btn_program, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    diag_ctrl.update(); // VIBRATION_SENSOR
    diag_ctrl.update(); // MAIN_VALVE
    diag_ctrl.update(); // SOFTENER_VALVE
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::SOFTENER_VALVE);

    // Click Start -> softener valve turns ON
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    EXPECT_CALL(valve_softener, turn_on()).Times(1);
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::SOFTENER_VALVE, 1, true)).Times(1);
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    diag_ctrl.update();
    EXPECT_TRUE(diag_ctrl.is_valve_active());
}

TEST_F(DiagnosticControllerTest, HighWaterLevelAutoShutsOffValveForOverflowSafety)
{
    diag_ctrl.enter();

    // Navigate to MAIN_VALVE (2 clicks on program)
    EXPECT_CALL(btn_program, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    diag_ctrl.update();
    diag_ctrl.update();

    // Click Start to turn ON valve
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    diag_ctrl.update();
    EXPECT_TRUE(diag_ctrl.is_valve_active());

    // Water level reaches HIGH_LEVEL -> automatically shuts OFF valve and sounds double beep
    EXPECT_CALL(water_sensor, get_current_level()).WillRepeatedly(Return(domain::WaterLevel::HIGH_LEVEL));
    EXPECT_CALL(buzzer, play_pattern(BuzzerPattern::DOUBLE_BEEP)).Times(1);
    EXPECT_CALL(valve_main, turn_off()).Times(1);

    diag_ctrl.update();
    EXPECT_FALSE(diag_ctrl.is_valve_active());
}

TEST_F(DiagnosticControllerTest, DrainPumpStepTogglesWithStartClick)
{
    diag_ctrl.enter();

    // Navigate to DRAIN_PUMP (4 clicks on program)
    EXPECT_CALL(btn_program, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    diag_ctrl.update(); // VIBRATION_SENSOR
    diag_ctrl.update(); // MAIN_VALVE
    diag_ctrl.update(); // SOFTENER_VALVE
    diag_ctrl.update(); // DRAIN_PUMP
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::DRAIN_PUMP);

    // Initial state: pump OFF
    EXPECT_FALSE(diag_ctrl.is_pump_active());

    // Click Start -> drain pump turns ON
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    EXPECT_CALL(water_sensor, get_current_level()).WillRepeatedly(Return(domain::WaterLevel::LOW_LEVEL));
    EXPECT_CALL(drain_pump, turn_on()).Times(1);
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::DRAIN_PUMP, static_cast<uint16_t>(domain::WaterLevel::LOW_LEVEL), true)).Times(1);
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    diag_ctrl.update();
    EXPECT_TRUE(diag_ctrl.is_pump_active());

    // Click Start again -> drain pump turns OFF
    EXPECT_CALL(drain_pump, turn_off()).Times(1);
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::DRAIN_PUMP, static_cast<uint16_t>(domain::WaterLevel::LOW_LEVEL), false)).Times(1);
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    diag_ctrl.update();
    EXPECT_FALSE(diag_ctrl.is_pump_active());
}

TEST_F(DiagnosticControllerTest, MotorAgitateStepTogglesWithStartClick)
{
    diag_ctrl.enter();

    // Navigate to MOTOR_AGITATE (5 clicks on program)
    EXPECT_CALL(btn_program, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    diag_ctrl.update(); // VIBRATION_SENSOR
    diag_ctrl.update(); // MAIN_VALVE
    diag_ctrl.update(); // SOFTENER_VALVE
    diag_ctrl.update(); // DRAIN_PUMP
    diag_ctrl.update(); // MOTOR_AGITATE
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::MOTOR_AGITATE);

    EXPECT_FALSE(diag_ctrl.is_agitate_active());

    // Click Start -> Starts agitation
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    EXPECT_CALL(motor, rotate_clockwise()).Times(1);
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    diag_ctrl.update();
    EXPECT_TRUE(diag_ctrl.is_agitate_active());

    // Click Start again -> Stops agitation
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    EXPECT_CALL(motor, stop()).Times(1);
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    diag_ctrl.update();
    EXPECT_FALSE(diag_ctrl.is_agitate_active());
}

TEST_F(DiagnosticControllerTest, SpinTestStepEngagesClutchThenRunsMotor)
{
    diag_ctrl.enter();

    // Navigate to SPIN_TEST (6 clicks on program)
    EXPECT_CALL(btn_program, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    diag_ctrl.update(); // VIBRATION_SENSOR
    diag_ctrl.update(); // MAIN_VALVE
    diag_ctrl.update(); // SOFTENER_VALVE
    diag_ctrl.update(); // DRAIN_PUMP
    diag_ctrl.update(); // MOTOR_AGITATE
    diag_ctrl.update(); // SPIN_TEST
    EXPECT_EQ(diag_ctrl.get_current_step(), DiagnosticStep::SPIN_TEST);

    EXPECT_FALSE(diag_ctrl.is_spin_active());

    // Click Start at t=1000ms -> Starts clutch engage (pump ON)
    EXPECT_CALL(timer_hal, get_time_ms()).WillRepeatedly(Return(1000));
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    EXPECT_CALL(drain_pump, turn_on()).Times(1);
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    diag_ctrl.update();
    EXPECT_TRUE(diag_ctrl.is_spin_active());

    // At t=4000ms (elapsed 3000ms < 5000ms): motor not yet rotating
    EXPECT_CALL(timer_hal, get_time_ms()).WillRepeatedly(Return(4000));
    EXPECT_CALL(motor, rotate_clockwise()).Times(0);
    diag_ctrl.update();

    // At t=6100ms (elapsed 5100ms >= 5000ms): motor rotates clockwise
    EXPECT_CALL(timer_hal, get_time_ms()).WillRepeatedly(Return(6100));
    EXPECT_CALL(motor, rotate_clockwise()).Times(1);
    diag_ctrl.update();

    // Click Start again -> stops spin test
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    EXPECT_CALL(motor, stop()).Times(1);
    EXPECT_CALL(drain_pump, turn_off()).Times(1);
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    diag_ctrl.update();
    EXPECT_FALSE(diag_ctrl.is_spin_active());
}

TEST_F(DiagnosticControllerTest, SpinTestTripsOnExcessiveVibrationAndLatchesWarning)
{
    diag_ctrl.enter();

    // Navigate to SPIN_TEST (6 clicks on program)
    EXPECT_CALL(btn_program, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));

    diag_ctrl.update();
    diag_ctrl.update();
    diag_ctrl.update();
    diag_ctrl.update();
    diag_ctrl.update();
    diag_ctrl.update();

    // Click Start to activate spin
    EXPECT_CALL(timer_hal, get_time_ms()).WillRepeatedly(Return(1000));
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::SPIN_TEST, 0x01, true)).Times(1);
    diag_ctrl.update();
    EXPECT_TRUE(diag_ctrl.is_spin_active());

    // Vibration trips monitor -> motor and pump immediately shut off and warning latched
    EXPECT_CALL(vib_monitor, get_vibration()).WillRepeatedly(Return(12500));
    EXPECT_CALL(motor, stop()).Times(1);
    EXPECT_CALL(drain_pump, turn_off()).Times(1);
    EXPECT_CALL(buzzer, play_pattern(BuzzerPattern::DOUBLE_BEEP)).Times(1);
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::SPIN_TEST, 0, false)).Times(1);

    diag_ctrl.update();

    EXPECT_FALSE(diag_ctrl.is_spin_active());
    EXPECT_TRUE(diag_ctrl.is_spin_tripped());

    // Click Start clears the trip warning
    EXPECT_CALL(btn_start, get_last_click())
        .WillOnce(Return(ButtonClickType::CLICK))
        .WillRepeatedly(Return(ButtonClickType::NONE_CLICK));
    EXPECT_CALL(buzzer, beep(30)).Times(1);
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::SPIN_TEST, 0, true)).Times(1);
    diag_ctrl.update();
    EXPECT_FALSE(diag_ctrl.is_spin_tripped());
}

TEST_F(DiagnosticControllerTest, InactiveControllerIgnoresUpdates)
{
    EXPECT_CALL(btn_program, get_last_click()).Times(0);
    EXPECT_CALL(btn_start, get_last_click()).Times(0);

    diag_ctrl.update();
}

TEST_F(DiagnosticControllerTest, SkipsLedPanelShowWhenBuzzerIsPlaying)
{
    diag_ctrl.enter();

    // When buzzer is playing audio, show_diagnostic is skipped to avoid interrupt collision
    EXPECT_CALL(buzzer, is_playing()).WillRepeatedly(Return(true));
    EXPECT_CALL(led_panel, show_diagnostic(_, _, _)).Times(0);

    diag_ctrl.update();

    // When buzzer finishes, show_diagnostic renders normally
    EXPECT_CALL(buzzer, is_playing()).WillRepeatedly(Return(false));
    EXPECT_CALL(led_panel, show_diagnostic(DiagnosticStep::LEVEL_SENSOR, _, _)).Times(1);

    diag_ctrl.update();
}
