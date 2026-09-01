#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_timer_hal.hpp"
#include "mocks/mock_reversible_motor.hpp"
#include "controllers/agitator.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;

class AgitatorTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockTimerHAL> mock_timer;
    NiceMock<mocks::MockReversibleMotor> mock_motor;

    uint32_t simulated_time_ms{0};

    controllers::Agitator agitator{mock_timer, mock_motor};

    void SetUp() override
    {
        simulated_time_ms = 0;
        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Invoke([this]() {
            return simulated_time_ms;
        }));
    }
};

TEST_F(AgitatorTest, StartsAgitationWithClockwisePulse)
{
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(1);

    agitator.start(10, 300, 200); // 10s, 300ms on, 200ms off
    EXPECT_TRUE(agitator.is_active());
    EXPECT_FALSE(agitator.is_finished());
}

TEST_F(AgitatorTest, AlternatesBetweenClockwiseAndCounterClockwiseWithOffPause)
{
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(1);
    agitator.start(10, 300, 200);

    // After 300ms ON -> motor stops
    simulated_time_ms = 300;
    EXPECT_CALL(mock_motor, stop()).Times(1);
    agitator.update();

    // After 200ms OFF -> CCW rotation starts
    simulated_time_ms = 500;
    EXPECT_CALL(mock_motor, rotate_counter_clockwise()).Times(1);
    agitator.update();

    // After 300ms ON -> motor stops
    simulated_time_ms = 800;
    EXPECT_CALL(mock_motor, stop()).Times(1);
    agitator.update();

    // After 200ms OFF -> CW rotation starts
    simulated_time_ms = 1000;
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(1);
    agitator.update();
}

TEST_F(AgitatorTest, FinishesAgitationWhenDurationExpires)
{
    agitator.start(2, 300, 200); // 2s duration

    simulated_time_ms = 2100;
    EXPECT_CALL(mock_motor, stop()).Times(1);

    agitator.update();
    EXPECT_FALSE(agitator.is_active());
    EXPECT_TRUE(agitator.is_finished());
}

TEST_F(AgitatorTest, PausesAndResumesWithoutLosingTiming)
{
    EXPECT_CALL(mock_motor, rotate_clockwise()).Times(::testing::AtLeast(1));
    EXPECT_CALL(mock_motor, stop()).Times(::testing::AtLeast(1));

    agitator.start(4, 300, 200); // 4s duration

    // Advance 1s and pause
    simulated_time_ms = 1000;
    agitator.pause();

    // While paused, updates do nothing
    simulated_time_ms = 5000;
    agitator.update();
    EXPECT_FALSE(agitator.is_finished());

    // Resume at 5000ms
    agitator.resume();

    // Need 3s more to complete 4s total
    simulated_time_ms = 7900;
    agitator.update();
    EXPECT_FALSE(agitator.is_finished());

    simulated_time_ms = 8100; // 1s + 3.1s = 4.1s
    agitator.update();
    EXPECT_TRUE(agitator.is_finished());
}
