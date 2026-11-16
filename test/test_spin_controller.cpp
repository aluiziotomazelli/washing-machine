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

    // Fast timings for unit tests (Step 5: accel/damping config, minimal mechanics)
    controllers::SpinConfig config{
        1000, // clutch engage
        1000, // duty on
        1000, // duty off
        2000, // coast down
        1000, // accel motor-on target
        5000, // accel wall cap
        100,  // damping min off
        500,  // damping max off
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
