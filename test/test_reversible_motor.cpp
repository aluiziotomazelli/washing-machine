#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_gpio_hal.hpp"
#include "mocks/mock_timer_hal.hpp"
#include "hal/reversible_motor.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::InSequence;
using ::testing::Invoke;

class ReversibleMotorTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockGpioHAL> mock_gpio;
    NiceMock<mocks::MockTimerHAL> mock_timer;

    const uint8_t pin_cw = 3;
    const uint8_t pin_ccw = 4;
    const uint16_t dead_time_ms = 200;

    hal::ReversibleMotor motor{mock_gpio, mock_timer, pin_cw, pin_ccw, dead_time_ms, false};

    uint32_t simulated_time_ms{0};

    void SetUp() override
    {
        simulated_time_ms = 0;
        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Invoke([this]() {
            return simulated_time_ms;
        }));

        motor.init();
    }

    void advance_time_ms(uint32_t ms)
    {
        simulated_time_ms += ms;
        motor.update();
    }
};

TEST_F(ReversibleMotorTest, InitializesInStoppedStateWithBothPinsDeEnergized)
{
    EXPECT_EQ(motor.get_state(), hal::MotorState::STOPPED);
}

TEST_F(ReversibleMotorTest, StartsClockwiseRotationImmediatelyFromStop)
{
    EXPECT_CALL(mock_gpio, set_level(pin_cw, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_ccw, hal::GpioLevel::LEVEL_LOW)).Times(::testing::AtLeast(1));

    motor.rotate_clockwise();

    EXPECT_EQ(motor.get_state(), hal::MotorState::RUNNING_CLOCKWISE);
}

TEST_F(ReversibleMotorTest, StartsCounterClockwiseRotationImmediatelyFromStop)
{
    EXPECT_CALL(mock_gpio, set_level(pin_ccw, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_cw, hal::GpioLevel::LEVEL_LOW)).Times(::testing::AtLeast(1));

    motor.rotate_counter_clockwise();

    EXPECT_EQ(motor.get_state(), hal::MotorState::RUNNING_COUNTER_CLOCKWISE);
}

TEST_F(ReversibleMotorTest, StopsImmediatelyWhenCommanded)
{
    motor.rotate_clockwise();
    EXPECT_EQ(motor.get_state(), hal::MotorState::RUNNING_CLOCKWISE);

    EXPECT_CALL(mock_gpio, set_level(pin_cw, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_ccw, hal::GpioLevel::LEVEL_LOW)).Times(1);

    motor.stop();
    EXPECT_EQ(motor.get_state(), hal::MotorState::STOPPED);
}

TEST_F(ReversibleMotorTest, EnforcesNonBlockingDeadTimeWhenReversingDirection)
{
    // 1. Start clockwise
    motor.rotate_clockwise();
    EXPECT_EQ(motor.get_state(), hal::MotorState::RUNNING_CLOCKWISE);

    // 2. Command reverse (Counter-Clockwise)
    // Both pins must de-energize immediately, and state becomes DEAD_TIME_WAIT
    EXPECT_CALL(mock_gpio, set_level(pin_cw, hal::GpioLevel::LEVEL_LOW)).Times(::testing::AtLeast(1));
    EXPECT_CALL(mock_gpio, set_level(pin_ccw, hal::GpioLevel::LEVEL_LOW)).Times(::testing::AtLeast(1));

    motor.rotate_counter_clockwise();
    EXPECT_EQ(motor.get_state(), hal::MotorState::DEAD_TIME_WAIT);

    // 3. Advance time by 100 ms (dead-time is 200 ms -> should STILL be in dead-time)
    advance_time_ms(100);
    EXPECT_EQ(motor.get_state(), hal::MotorState::DEAD_TIME_WAIT);

    // 4. Advance time past 200 ms threshold (advance another 110 ms -> 210 ms total)
    EXPECT_CALL(mock_gpio, set_level(pin_ccw, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    advance_time_ms(110);

    EXPECT_EQ(motor.get_state(), hal::MotorState::RUNNING_COUNTER_CLOCKWISE);
}

TEST_F(ReversibleMotorTest, CancelsPendingReversalIfEmergencyStopOccursDuringDeadTime)
{
    motor.rotate_clockwise();
    motor.rotate_counter_clockwise();
    EXPECT_EQ(motor.get_state(), hal::MotorState::DEAD_TIME_WAIT);

    // Emergency stop called while waiting in dead-time
    motor.stop();
    EXPECT_EQ(motor.get_state(), hal::MotorState::STOPPED);

    // After dead-time elapses, CCW must NEVER be energized
    EXPECT_CALL(mock_gpio, set_level(pin_ccw, hal::GpioLevel::LEVEL_HIGH)).Times(0);
    advance_time_ms(300);

    EXPECT_EQ(motor.get_state(), hal::MotorState::STOPPED);
}
