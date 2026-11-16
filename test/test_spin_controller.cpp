#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_timer_hal.hpp"
#include "mocks/mock_digital_output.hpp"
#include "mocks/mock_reversible_motor.hpp"
#include "mocks/mock_vibration_monitor.hpp"
#include "controllers/spin_controller.hpp"

using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

class SpinControllerTest : public ::testing::Test
{
protected:
    NiceMock<mocks::MockTimerHAL> mock_timer;
    NiceMock<mocks::MockDigitalOutput> mock_drain_pump;
    NiceMock<mocks::MockReversibleMotor> mock_motor;

    uint32_t simulated_time_ms{0};

    // Fast timings for unit tests (accel/damping config)
    controllers::SpinConfig config{
        1000, // clutch engage
        1000, // duty on
        1000, // duty off
        2000, // coast down
        1000, // accel motor-on target
        5000, // accel wall cap
        100,  // damping min off
        500,  // damping max off
        200,  // damping sensor timeout
        2     // max unbalance retries
    };

    controllers::SpinController spin_ctrl{mock_timer, mock_drain_pump, mock_motor, config};

    void SetUp() override
    {
        simulated_time_ms = 0;
        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Invoke([this]() { return simulated_time_ms; }));
    }

    void step_to_duty_run()
    {
        // Clutch engage completes -> ACCEL, then ACCEL completes by motor-on target
        simulated_time_ms += 1000;
        spin_ctrl.update(); // clutch -> ACCEL, starts motor
        simulated_time_ms += 1000;
        spin_ctrl.update(); // ACCEL target reached -> DUTY_RUN_ON
    }
};

TEST_F(SpinControllerTest, StartsClutchEngagementAndTurnsPumpOn)
{
    EXPECT_CALL(mock_drain_pump, turn_on()).Times(1);
    EXPECT_CALL(mock_motor, stop()).Times(1);

    spin_ctrl.start(domain::WaterLevel::LOW_LEVEL, 60);
    EXPECT_TRUE(spin_ctrl.is_active());
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::CLUTCH_ENGAGE);
}

TEST_F(SpinControllerTest, TransitionsFromClutchToAccelAfterDelay)
{
    spin_ctrl.start(domain::WaterLevel::LOW_LEVEL, 60);

    // 500ms into 1000ms clutch delay -> still in clutch engage
    simulated_time_ms = 500;
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::CLUTCH_ENGAGE);

    // 1000ms reached -> starts ACCEL (CW)
    simulated_time_ms = 1000;
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(1);
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::ACCEL);
}

TEST_F(SpinControllerTest, AccelTransitionsToDutyRunAfterMotorOnTarget)
{
    spin_ctrl.start(domain::WaterLevel::LOW_LEVEL, 60);

    // Clutch -> ACCEL
    simulated_time_ms += 1000;
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::ACCEL);

    // ACCEL motor-on target reached
    simulated_time_ms += 1000;
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(AtLeast(1));
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_ON);
}

TEST_F(SpinControllerTest, DutyRunAlternatesOnAndOffAndEntersCoastDown)
{
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(AtLeast(1));
    EXPECT_CALL(mock_motor, stop()).Times(AtLeast(1));

    // Start with 2s duty run duration
    spin_ctrl.start(domain::WaterLevel::LOW_LEVEL, 2);
    step_to_duty_run();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_ON);

    // 1s Duty On
    simulated_time_ms += 1000;
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_OFF);

    // 1s Duty Off -> 2s duty duration reached!
    simulated_time_ms += 1000;
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::COAST_DOWN);

    // Coast down 2s -> stops pump and finishes!
    simulated_time_ms += 2000;
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    spin_ctrl.update();
    EXPECT_TRUE(spin_ctrl.is_finished());
    EXPECT_FALSE(spin_ctrl.is_active());
}

TEST_F(SpinControllerTest, PauseDuringSpinKeepsPumpOnUntilCoastDownCompletes)
{
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(AtLeast(1));
    EXPECT_CALL(mock_motor, stop()).Times(AtLeast(1));

    spin_ctrl.start(domain::WaterLevel::LOW_LEVEL, 60);
    step_to_duty_run();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_ON);

    // Pause while spinning
    spin_ctrl.pause();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::PAUSE_COASTING);

    // 1s into coast-down: pump still ON, motor stopped
    simulated_time_ms += 1000;
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::PAUSE_COASTING);

    // 2s coast-down finishes: pump turns off and officially pauses
    simulated_time_ms += 1000;
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::PAUSED);
    EXPECT_TRUE(spin_ctrl.is_paused());

    // Resume re-engages clutch and pump safely
    EXPECT_CALL(mock_drain_pump, turn_on()).Times(1);
    spin_ctrl.resume();
    EXPECT_FALSE(spin_ctrl.is_paused());
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::CLUTCH_ENGAGE);

    // After clutch engages, it restarts from ACCEL
    simulated_time_ms += 1000;
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(1);
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::ACCEL);
}

TEST_F(SpinControllerTest, StopDuringSpinKeepsPumpOnUntilCoastDownCompletes)
{
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(AtLeast(1));
    EXPECT_CALL(mock_motor, stop()).Times(AtLeast(1));

    spin_ctrl.start(domain::WaterLevel::LOW_LEVEL, 60);
    step_to_duty_run();

    // Soft stop
    spin_ctrl.stop();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::STOP_COASTING);

    // Finish 2s coast down
    simulated_time_ms += 2000;
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    spin_ctrl.update();
    EXPECT_FALSE(spin_ctrl.is_active());
    EXPECT_TRUE(spin_ctrl.is_finished());
}

TEST_F(SpinControllerTest, EmergencyStopCutsBothImmediately)
{
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(AtLeast(1));
    EXPECT_CALL(mock_motor, stop()).Times(AtLeast(1));

    spin_ctrl.start(domain::WaterLevel::LOW_LEVEL, 60);
    step_to_duty_run();

    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    spin_ctrl.emergency_stop();
    EXPECT_FALSE(spin_ctrl.is_active());
}

TEST_F(SpinControllerTest, NullVibrationMonitorOperatesNormally)
{
    // spin_ctrl in fixture has vibration_monitor = nullptr
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(AtLeast(1));
    EXPECT_CALL(mock_drain_pump, turn_on()).Times(1);

    spin_ctrl.start(domain::WaterLevel::LOW_LEVEL, 60);
    EXPECT_FALSE(spin_ctrl.has_error());
    EXPECT_EQ(spin_ctrl.get_unbalance_retries(), 0);

    // Update through clutch -> ACCEL
    simulated_time_ms += 1000;
    spin_ctrl.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::ACCEL);
    EXPECT_FALSE(spin_ctrl.has_error());
}

TEST_F(SpinControllerTest, UnbalanceTripEntersRetryCoastingAndRestartsAccel)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig retry_cfg = config;
    retry_cfg.max_unbalance_retries = 2;
    retry_cfg.coast_down_ms = 2000;

    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, retry_cfg, &mock_vib};

    EXPECT_CALL(mock_vib, reset()).Times(AtLeast(1));
    EXPECT_CALL(mock_drain_pump, turn_on()).Times(1);
    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);

    // Clutch engage (1000 ms) finishes and transitions to ACCEL
    simulated_time_ms += 1000;
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(1);
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::ACCEL);

    // Normal spinning without unbalance
    simulated_time_ms += 100;
    EXPECT_CALL(mock_vib, update()).Times(1);
    EXPECT_CALL(mock_vib, is_critical_unbalance()).WillOnce(Return(false));
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::ACCEL);

    // Unbalance trip occurs during ACCEL
    simulated_time_ms += 500;
    EXPECT_CALL(mock_vib, update()).Times(1);
    EXPECT_CALL(mock_vib, is_critical_unbalance()).WillOnce(Return(true));
    EXPECT_CALL(mock_motor, stop()).Times(AtLeast(1));

    spin_with_vib.update();

    // Enters RETRY_COASTING, not error
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::RETRY_COASTING);
    EXPECT_EQ(spin_with_vib.get_unbalance_retries(), 1);
    EXPECT_FALSE(spin_with_vib.has_error());
    EXPECT_TRUE(spin_with_vib.is_active());

    // During coast down (1000ms), pump stays ON
    simulated_time_ms += 1000;
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::RETRY_COASTING);

    // Coast down completes (2000ms elapsed) -> resets monitor & restarts from CLUTCH_ENGAGE
    simulated_time_ms += 1000;
    EXPECT_CALL(mock_vib, reset()).Times(1);
    spin_with_vib.update();

    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::CLUTCH_ENGAGE);
    EXPECT_FALSE(spin_with_vib.has_error());
    EXPECT_EQ(spin_with_vib.get_unbalance_retries(), 1);

    // Clutch engage finishes -> transitions to ACCEL
    simulated_time_ms += 1000;
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(1);
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::ACCEL);
}

TEST_F(SpinControllerTest, UnbalanceTripExhaustingRetriesEntersStopCoastingAndFlagsError)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig retry_cfg = config;
    retry_cfg.max_unbalance_retries = 1; // Only 1 retry allowed
    retry_cfg.coast_down_ms = 2000;

    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, retry_cfg, &mock_vib};

    EXPECT_CALL(mock_vib, reset()).Times(AtLeast(1));
    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);

    // Clutch -> ACCEL
    simulated_time_ms += 1000;
    spin_with_vib.update();

    // 1st trip: Retry 1
    simulated_time_ms += 500;
    EXPECT_CALL(mock_vib, update()).Times(1);
    EXPECT_CALL(mock_vib, is_critical_unbalance()).WillOnce(Return(true));
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::RETRY_COASTING);
    EXPECT_EQ(spin_with_vib.get_unbalance_retries(), 1);
    EXPECT_FALSE(spin_with_vib.has_error());

    // Coast down expires -> Re-enters CLUTCH_ENGAGE
    simulated_time_ms += 2000;
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::CLUTCH_ENGAGE);

    // Clutch expires -> enters ACCEL again
    simulated_time_ms += 1000;
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::ACCEL);

    // 2nd trip: Retries exhausted (1 >= max 1)
    simulated_time_ms += 500;
    EXPECT_CALL(mock_vib, update()).Times(1);
    EXPECT_CALL(mock_vib, is_critical_unbalance()).WillOnce(Return(true));
    EXPECT_CALL(mock_motor, stop()).Times(AtLeast(1));
    spin_with_vib.update();

    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::STOP_COASTING);
    EXPECT_FALSE(spin_with_vib.has_error());
    EXPECT_TRUE(spin_with_vib.is_active());

    // Finish coast-down to safe complete stop
    simulated_time_ms += 2000;
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    spin_with_vib.update();

    EXPECT_FALSE(spin_with_vib.is_active());
    EXPECT_TRUE(spin_with_vib.has_error());
}

TEST_F(SpinControllerTest, NewStartResetsRetriesAndError)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig retry_cfg = config;
    retry_cfg.max_unbalance_retries = 0; // Immediate error on trip
    retry_cfg.coast_down_ms = 2000;

    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, retry_cfg, &mock_vib};

    EXPECT_CALL(mock_vib, reset()).Times(AtLeast(1));
    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);

    // Trip immediately during ACCEL
    simulated_time_ms += 1000;
    spin_with_vib.update();
    EXPECT_CALL(mock_vib, update()).Times(1);
    EXPECT_CALL(mock_vib, is_critical_unbalance()).WillOnce(Return(true));
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::STOP_COASTING);

    // Complete coast-down
    simulated_time_ms += 2000;
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    spin_with_vib.update();
    EXPECT_TRUE(spin_with_vib.has_error());

    // Re-start clears error and retry counter
    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    EXPECT_FALSE(spin_with_vib.has_error());
    EXPECT_EQ(spin_with_vib.get_unbalance_retries(), 0);
}

TEST_F(SpinControllerTest, ResetErrorExplicitlyClearsErrorAndRetries)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig retry_cfg = config;
    retry_cfg.max_unbalance_retries = 0; // Immediate error on trip
    retry_cfg.coast_down_ms = 2000;

    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, retry_cfg, &mock_vib};

    EXPECT_CALL(mock_vib, reset()).Times(AtLeast(1));
    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);

    // Trip into error
    simulated_time_ms += 1000;
    spin_with_vib.update();
    EXPECT_CALL(mock_vib, update()).Times(1);
    EXPECT_CALL(mock_vib, is_critical_unbalance()).WillOnce(Return(true));
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::STOP_COASTING);

    // Complete coast-down
    simulated_time_ms += 2000;
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    spin_with_vib.update();
    EXPECT_TRUE(spin_with_vib.has_error());

    // Explicit reset_error() clears error and monitor
    EXPECT_CALL(mock_vib, reset()).Times(1);
    spin_with_vib.reset_error();

    EXPECT_FALSE(spin_with_vib.has_error());
    EXPECT_EQ(spin_with_vib.get_unbalance_retries(), 0);
}

TEST_F(SpinControllerTest, WarningDuringAccelEntersVibeDampingAndStopsMotor)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, config, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    simulated_time_ms += 1000;
    spin_with_vib.update(); // CLUTCH -> ACCEL
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::ACCEL);

    EXPECT_CALL(mock_vib, update()).Times(AtLeast(1));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(true));
    EXPECT_CALL(mock_motor, stop()).Times(AtLeast(1));

    simulated_time_ms += 100;
    spin_with_vib.update();

    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);
    EXPECT_TRUE(spin_with_vib.is_active());
    EXPECT_FALSE(spin_with_vib.has_error());
}

TEST_F(SpinControllerTest, VibeDampingContinuesSamplingMonitorAndResumesToAccel)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, config, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    simulated_time_ms += 1000;
    spin_with_vib.update(); // -> ACCEL

    // Enter damping on warning (damping starts at t=1100)
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(true));
    simulated_time_ms += 100;
    spin_with_vib.update();
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    // While damping, warning persists and monitor must be sampled (deadlock guard)
    EXPECT_CALL(mock_vib, update()).Times(AtLeast(3));
    simulated_time_ms += 50;
    spin_with_vib.update();
    simulated_time_ms += 20;
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    // Warning clears but min_off not yet elapsed (elapsed 70ms < 100ms)
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));
    simulated_time_ms += 0; // still at t=1170
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    // After min_off elapsed, resumes to ACCEL (t=1210, elapsed 110ms)
    simulated_time_ms += 40;
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(AtLeast(1));
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::ACCEL);
}

TEST_F(SpinControllerTest, WarningDuringDutyRunOnEntersVibeDampingAndResumesDutyRunOn)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig cfg = config;
    cfg.accel_motor_on_ms = 100; // reach duty quickly
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, cfg, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    simulated_time_ms += 1000;
    spin_with_vib.update(); // -> ACCEL
    simulated_time_ms += 100;
    spin_with_vib.update(); // -> DUTY_RUN_ON
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_ON);

    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(true));
    simulated_time_ms += 50;
    spin_with_vib.update();
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));
    simulated_time_ms += 150; // >= min_off
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(AtLeast(1));
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_ON);
}

TEST_F(SpinControllerTest, WarningDuringDutyRunOffDoesNotEnterVibeDamping)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig cfg = config;
    cfg.accel_motor_on_ms = 100;
    cfg.duty_on_ms = 200;
    cfg.duty_off_ms = 500;
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, cfg, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    simulated_time_ms += 1000;
    spin_with_vib.update(); // -> ACCEL
    simulated_time_ms += 100;
    spin_with_vib.update(); // -> DUTY_RUN_ON
    simulated_time_ms += 200;
    spin_with_vib.update(); // -> DUTY_RUN_OFF
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_OFF);

    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(true));
    EXPECT_CALL(mock_vib, update()).Times(AtLeast(1));

    simulated_time_ms += 50;
    spin_with_vib.update();

    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_OFF);
}

TEST_F(SpinControllerTest, CriticalDuringVibeDampingEntersRetryCoasting)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig cfg = config;
    cfg.max_unbalance_retries = 1;
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, cfg, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    simulated_time_ms += 1000;
    spin_with_vib.update(); // -> ACCEL

    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(true));
    simulated_time_ms += 100;
    spin_with_vib.update();
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(true));
    EXPECT_CALL(mock_motor, stop()).Times(AtLeast(1));
    simulated_time_ms += 50;
    spin_with_vib.update();

    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::RETRY_COASTING);
    EXPECT_EQ(spin_with_vib.get_unbalance_retries(), 1);
}

TEST_F(SpinControllerTest, DampingMaxOffEscalatesToRetryOrStop)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig cfg = config;
    cfg.damping_max_off_ms = 300;
    cfg.max_unbalance_retries = 1;
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, cfg, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    simulated_time_ms += 1000;
    spin_with_vib.update(); // -> ACCEL

    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(true));
    simulated_time_ms += 100;
    spin_with_vib.update();
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    // Keep warning active past max_off
    simulated_time_ms += 350; // >= max_off
    spin_with_vib.update();

    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::RETRY_COASTING);
}

TEST_F(SpinControllerTest, SensorFailureDuringDampingEscalatesAfterTimeout)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig cfg = config;
    cfg.damping_sensor_timeout_ms = 150;
    cfg.max_unbalance_retries = 1;
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, cfg, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    simulated_time_ms += 1000;
    spin_with_vib.update(); // -> ACCEL

    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(true));
    simulated_time_ms += 100;
    spin_with_vib.update();
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    // Sensor fails continuously
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(false));
    simulated_time_ms += 100; // first bad sample seen here, duration ~0
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    simulated_time_ms += 160; // timeout exceeded
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::RETRY_COASTING);
}

TEST_F(SpinControllerTest, PauseDuringVibeDampingAndResumeRestartsFromAccelSafely)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, config, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    simulated_time_ms += 1000;
    spin_with_vib.update(); // -> ACCEL

    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(true));
    simulated_time_ms += 100;
    spin_with_vib.update();
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    spin_with_vib.pause();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::PAUSE_COASTING);

    simulated_time_ms += config.coast_down_ms;
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::PAUSED);

    EXPECT_CALL(mock_drain_pump, turn_on()).Times(AtLeast(1));
    spin_with_vib.resume();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::CLUTCH_ENGAGE);

    simulated_time_ms += config.clutch_engage_ms;
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::ACCEL);
}

TEST_F(SpinControllerTest, DutyRunElapsedExcludesVibeDampingTime)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig cfg = config;
    cfg.accel_motor_on_ms = 100;
    cfg.duty_on_ms = 300;
    cfg.duty_off_ms = 1000;
    cfg.damping_min_off_ms = 50;
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, cfg, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 1); // 1000ms duty

    simulated_time_ms += 1000;
    spin_with_vib.update(); // clutch -> ACCEL
    simulated_time_ms += 100;
    spin_with_vib.update(); // -> DUTY_RUN_ON
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_ON);

    // 100ms into duty ON, enter damping
    simulated_time_ms += 100;
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(true));
    spin_with_vib.update();
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);

    // stay in damping for 500ms (should not count toward duty)
    simulated_time_ms += 500;
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));
    spin_with_vib.update(); // resumes to DUTY_RUN_ON
    ASSERT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_ON);

    // remaining 200ms of ON pulse should complete now
    simulated_time_ms += 200;
    spin_with_vib.update();
    EXPECT_EQ(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::DUTY_RUN_OFF);
}

TEST_F(SpinControllerTest, BalancedLoadNeverEntersVibeDamping)
{
    mocks::MockVibrationMonitor mock_vib;
    controllers::SpinConfig cfg = config;
    cfg.accel_motor_on_ms = 200;
    controllers::SpinController spin_with_vib{mock_timer, mock_drain_pump, mock_motor, cfg, &mock_vib};

    ON_CALL(mock_vib, is_sensor_ok()).WillByDefault(Return(true));
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    ON_CALL(mock_vib, is_warning()).WillByDefault(Return(false));

    spin_with_vib.start(domain::WaterLevel::LOW_LEVEL, 60);
    for (int i = 0; i < 20; ++i) {
        simulated_time_ms += 100;
        spin_with_vib.update();
        ASSERT_NE(spin_with_vib.get_sub_phase(), controllers::SpinSubPhase::VIBE_DAMPING);
    }
}
