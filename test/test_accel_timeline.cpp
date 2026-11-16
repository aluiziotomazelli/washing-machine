#include <gtest/gtest.h>
#include "controllers/accel_timeline.hpp"

using controllers::AccelTimeline;

class AccelTimelineTest : public ::testing::Test
{
protected:
    AccelTimeline timeline;
};

TEST_F(AccelTimelineTest, IsInactiveAndNotCompleteBeforeStart)
{
    EXPECT_FALSE(timeline.is_active());
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 0u);
    EXPECT_FALSE(timeline.is_complete(0));
    EXPECT_FALSE(timeline.is_complete(100000));
}

TEST_F(AccelTimelineTest, StartActivatesTimeline)
{
    timeline.start(1000, 5000, 20000);

    EXPECT_TRUE(timeline.is_active());
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 0u);
    EXPECT_FALSE(timeline.is_complete(1000));
}

TEST_F(AccelTimelineTest, MotorOnAccumulatesOnlyWhileMotorIsOn)
{
    // Start at t = 1000 ms with target 20000 ms and wall cap 40000 ms
    timeline.start(1000, 20000, 40000);
    timeline.on_motor_on(1000);

    // After 5000 ms of motor on (t = 6000)
    timeline.on_motor_on(6000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 5000u);
    EXPECT_FALSE(timeline.is_complete(6000));

    // Motor turns off (e.g. entering VIBE_DAMPING) at t = 6000
    timeline.on_motor_off(6000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 5000u);

    // Time passes during damping for 3000 ms (t = 9000)
    // Motor off time must not accumulate
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 5000u);
    EXPECT_FALSE(timeline.is_complete(9000));

    // Motor resumes at t = 9000
    timeline.on_motor_on(9000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 5000u);

    // Motor runs for another 15000 ms (t = 24000)
    timeline.on_motor_on(24000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 20000u);
    EXPECT_TRUE(timeline.is_complete(24000));
}

TEST_F(AccelTimelineTest, MotorOffTimeDoesNotAccumulate)
{
    timeline.start(0, 10000, 30000);

    // Motor starts OFF, 5000 ms passes
    timeline.on_motor_off(5000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 0u);

    // Turn motor ON at t = 5000
    timeline.on_motor_on(5000);
    // Runs for 2000 ms (t = 7000)
    timeline.on_motor_on(7000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 2000u);

    // Turn motor OFF at t = 7000, stays OFF until t = 12000
    timeline.on_motor_off(7000);
    timeline.on_motor_off(12000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 2000u);
}

TEST_F(AccelTimelineTest, CompletionOccursAtMotorOnTarget)
{
    timeline.start(100, 5000, 20000);
    timeline.on_motor_on(100);

    timeline.on_motor_on(5000);
    EXPECT_FALSE(timeline.is_complete(5000)); // 4900 ms elapsed

    timeline.on_motor_on(5100);
    EXPECT_TRUE(timeline.is_complete(5100)); // 5000 ms elapsed -> complete!
}

TEST_F(AccelTimelineTest, WallClockCapCompletesPhaseEvenIfMotorOnTimeIsShort)
{
    // Motor-on target is 20000 ms, wall cap is 30000 ms
    timeline.start(0, 20000, 30000);
    timeline.on_motor_on(0);

    // Motor runs for only 2000 ms, then is paused for long damping
    timeline.on_motor_on(2000);
    timeline.on_motor_off(2000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 2000u);

    // At t = 29999, not yet wall cap
    EXPECT_FALSE(timeline.is_complete(29999));

    // At t = 30000, wall cap is reached -> must complete!
    EXPECT_TRUE(timeline.is_complete(30000));
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 2000u); // Motor-on was short
}

TEST_F(AccelTimelineTest, RestartAfterCompletionResetsActiveSession)
{
    timeline.start(1000, 5000, 10000);
    timeline.on_motor_on(1000);
    timeline.on_motor_on(6000);
    EXPECT_TRUE(timeline.is_active());
    EXPECT_TRUE(timeline.is_complete(6000));

    // Restart at t = 10000
    timeline.start(10000, 8000, 15000);
    EXPECT_TRUE(timeline.is_active());
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 0u);
    EXPECT_FALSE(timeline.is_complete(10000));

    timeline.on_motor_on(10000);
    timeline.on_motor_on(14000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 4000u);
    EXPECT_FALSE(timeline.is_complete(14000));
}

TEST_F(AccelTimelineTest, ResetMakesTimelineInactiveAndClearsProgress)
{
    timeline.start(1000, 5000, 10000);
    timeline.on_motor_on(1000);
    timeline.on_motor_on(6000);
    EXPECT_TRUE(timeline.is_complete(6000));

    timeline.reset();

    EXPECT_FALSE(timeline.is_active());
    EXPECT_FALSE(timeline.is_motor_on());
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 0u);
    EXPECT_FALSE(timeline.is_complete(6000));
    EXPECT_FALSE(timeline.is_complete(100000));
}

TEST_F(AccelTimelineTest, IsCompleteFalseAfterResetEvenWhenWallCapWouldHaveElapsed)
{
    timeline.start(0, 20000, 30000);
    timeline.on_motor_on(0);
    timeline.on_motor_on(2000);
    timeline.on_motor_off(2000);
    EXPECT_EQ(timeline.motor_on_elapsed_ms(), 2000u);

    timeline.reset();

    EXPECT_FALSE(timeline.is_active());
    EXPECT_FALSE(timeline.is_complete(30000));
    EXPECT_FALSE(timeline.is_complete(50000));
}
