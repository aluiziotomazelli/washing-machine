#include <gtest/gtest.h>
#include "controllers/vibration_damping_policy.hpp"

using controllers::DampingAction;
using controllers::DampingConfig;
using controllers::DampingInput;
using controllers::VibrationDampingPolicy;

class VibrationDampingPolicyTest : public ::testing::Test
{
protected:
    DampingConfig config{
        800,  // min_off_ms
        8000, // max_off_ms
        1000  // sensor_timeout_ms
    };

    VibrationDampingPolicy policy{config};
};

TEST_F(VibrationDampingPolicyTest, CriticalInputReturnsEscalate)
{
    DampingInput input;
    input.sensor_ok = true;
    input.warning = true;
    input.critical = true;

    // Critical always escalates whether motor is energized or damping is active
    EXPECT_EQ(policy.evaluate(input, true, false), DampingAction::ESCALATE);
    EXPECT_EQ(policy.evaluate(input, false, true), DampingAction::ESCALATE);
    EXPECT_EQ(policy.evaluate(input, false, false), DampingAction::ESCALATE);
}

TEST_F(VibrationDampingPolicyTest, SensorTimeoutReturnsEscalate)
{
    DampingInput input;
    input.sensor_ok = false;
    input.warning = false;
    input.critical = false;
    input.sensor_bad_duration_ms = 999;

    // Before timeout, does not escalate immediately on sensor glitch
    EXPECT_NE(policy.evaluate(input, true, false), DampingAction::ESCALATE);

    // At or after timeout, escalates
    input.sensor_bad_duration_ms = 1000;
    EXPECT_EQ(policy.evaluate(input, true, false), DampingAction::ESCALATE);

    input.sensor_bad_duration_ms = 2500;
    EXPECT_EQ(policy.evaluate(input, false, true), DampingAction::ESCALATE);
}

TEST_F(VibrationDampingPolicyTest, WarningWithMotorEnergizedReturnsPause)
{
    DampingInput input;
    input.sensor_ok = true;
    input.warning = true;
    input.critical = false;

    // Motor is energized (ACCEL or DUTY_RUN_ON) -> must PAUSE
    EXPECT_EQ(policy.evaluate(input, true, false), DampingAction::PAUSE);
}

TEST_F(VibrationDampingPolicyTest, WarningWithMotorDeenergizedDoesNotReturnPause)
{
    DampingInput input;
    input.sensor_ok = true;
    input.warning = true;
    input.critical = false;

    // Motor is off (DUTY_RUN_OFF) and not in active damping -> should not PAUSE
    EXPECT_NE(policy.evaluate(input, false, false), DampingAction::PAUSE);
    EXPECT_EQ(policy.evaluate(input, false, false), DampingAction::NONE);
}

TEST_F(VibrationDampingPolicyTest, DampingActiveWarningClearBeforeMinOffReturnsNone)
{
    DampingInput input;
    input.sensor_ok = true;
    input.warning = false; // Warning cleared
    input.critical = false;
    input.damping_elapsed_ms = 500; // < min_off_ms (800)

    // Cannot resume yet, so action is NONE
    EXPECT_EQ(policy.evaluate(input, false, true), DampingAction::NONE);
    EXPECT_FALSE(policy.can_resume(input, input.damping_elapsed_ms));
}

TEST_F(VibrationDampingPolicyTest, DampingActiveWarningClearAfterMinOffReturnsResume)
{
    DampingInput input;
    input.sensor_ok = true;
    input.warning = false; // Warning cleared
    input.critical = false;
    input.damping_elapsed_ms = 800; // == min_off_ms (800)

    EXPECT_EQ(policy.evaluate(input, false, true), DampingAction::RESUME);
    EXPECT_TRUE(policy.can_resume(input, input.damping_elapsed_ms));

    input.damping_elapsed_ms = 1200; // > min_off_ms
    EXPECT_EQ(policy.evaluate(input, false, true), DampingAction::RESUME);
    EXPECT_TRUE(policy.can_resume(input, input.damping_elapsed_ms));
}

TEST_F(VibrationDampingPolicyTest, DampingActiveMaxOffExceededReturnsEscalate)
{
    DampingInput input;
    input.sensor_ok = true;
    input.warning = true; // Warning persisted
    input.critical = false;
    input.damping_elapsed_ms = 8000; // >= max_off_ms (8000)

    EXPECT_EQ(policy.evaluate(input, false, true), DampingAction::ESCALATE);
    EXPECT_FALSE(policy.can_resume(input, input.damping_elapsed_ms));
}

TEST_F(VibrationDampingPolicyTest, NormalOperationReturnsNone)
{
    DampingInput input;
    input.sensor_ok = true;
    input.warning = false;
    input.critical = false;

    EXPECT_EQ(policy.evaluate(input, true, false), DampingAction::NONE);
    EXPECT_EQ(policy.evaluate(input, false, false), DampingAction::NONE);
}

TEST_F(VibrationDampingPolicyTest, RejectsOrClampsInvalidConfiguration)
{
    DampingConfig invalid{8000, 8000, 0}; // min >= max and zero timeout
    VibrationDampingPolicy invalid_policy{invalid};

    EXPECT_FALSE(invalid_policy.is_config_valid());
    const DampingConfig& cfg = invalid_policy.get_config();
    EXPECT_LT(cfg.min_off_ms, cfg.max_off_ms);
    EXPECT_GT(cfg.sensor_timeout_ms, 0u);
}
