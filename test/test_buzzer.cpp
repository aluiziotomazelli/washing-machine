#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_gpio_hal.hpp"
#include "mocks/mock_timer_hal.hpp"
#include "ui/buzzer.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::Invoke;
using ::testing::InSequence;

class BuzzerTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockGpioHAL> mock_gpio;
    NiceMock<mocks::MockTimerHAL> mock_timer;

    const uint8_t pin = 5;
    const uint16_t freq_hz = 3000;
    ui::Buzzer buzzer{mock_gpio, mock_timer, pin, freq_hz};

    uint32_t simulated_time_ms{0};

    void SetUp() override
    {
        simulated_time_ms = 0;
        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Invoke([this]() {
            return simulated_time_ms;
        }));

        buzzer.init();
    }

    void advance_time_ms(uint32_t ms)
    {
        simulated_time_ms += ms;
        buzzer.update();
    }
};

TEST_F(BuzzerTest, InitializesSilentAndConfiguresOutputPin)
{
    EXPECT_FALSE(buzzer.is_playing());
}

TEST_F(BuzzerTest, PlaysSingleBeepAt3000HzAndStopsAfterDuration)
{
    EXPECT_CALL(mock_gpio, play_tone(pin, freq_hz)).Times(1);
    buzzer.beep(50);
    EXPECT_TRUE(buzzer.is_playing());

    // Advance 50ms (duration elapsed)
    EXPECT_CALL(mock_gpio, stop_tone(pin)).Times(1);
    advance_time_ms(50);

    EXPECT_FALSE(buzzer.is_playing());
}

TEST_F(BuzzerTest, PlaysDoubleBeepSequenceAt3000Hz)
{
    InSequence seq;

    // 1st Beep ON (60ms)
    EXPECT_CALL(mock_gpio, play_tone(pin, freq_hz)).Times(1);
    buzzer.play_pattern(ui::BuzzerPattern::DOUBLE_BEEP);
    EXPECT_TRUE(buzzer.is_playing());

    // 1st Beep OFF (60ms pause)
    EXPECT_CALL(mock_gpio, stop_tone(pin)).Times(1);
    advance_time_ms(60);
    EXPECT_TRUE(buzzer.is_playing());

    // 2nd Beep ON (60ms)
    EXPECT_CALL(mock_gpio, play_tone(pin, freq_hz)).Times(1);
    advance_time_ms(60);
    EXPECT_TRUE(buzzer.is_playing());

    // 2nd Beep OFF (sequence finished)
    EXPECT_CALL(mock_gpio, stop_tone(pin)).Times(1);
    advance_time_ms(60);

    EXPECT_FALSE(buzzer.is_playing());
}

TEST_F(BuzzerTest, PlaysContinuousErrorAlarmUntilStopped)
{
    buzzer.play_pattern(ui::BuzzerPattern::ERROR_ALARM);
    EXPECT_TRUE(buzzer.is_playing());

    // Cycle through several alarm intervals (250ms ON, 250ms OFF)
    advance_time_ms(250); // OFF
    EXPECT_TRUE(buzzer.is_playing());

    advance_time_ms(250); // ON
    EXPECT_TRUE(buzzer.is_playing());

    advance_time_ms(250); // OFF
    EXPECT_TRUE(buzzer.is_playing());

    // Explicit stop shuts down continuous alarm
    EXPECT_CALL(mock_gpio, stop_tone(pin)).Times(1);
    buzzer.stop();

    EXPECT_FALSE(buzzer.is_playing());
}

TEST_F(BuzzerTest, StopImmediatelySilencesOngoingTone)
{
    buzzer.beep(500);
    EXPECT_TRUE(buzzer.is_playing());

    EXPECT_CALL(mock_gpio, stop_tone(pin)).Times(1);
    buzzer.stop();

    EXPECT_FALSE(buzzer.is_playing());
}
