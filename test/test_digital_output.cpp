#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_gpio_hal.hpp"
#include "hal/digital_output.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

class DigitalOutputTest : public ::testing::Test
{
protected:
    NiceMock<mocks::MockGpioHAL> mock_gpio;

    void SetUp() override { hal::DigitalOutput::reset_registry(); }

    void TearDown() override { hal::DigitalOutput::reset_registry(); }
};

TEST_F(DigitalOutputTest, InitializesPinModeAndDefaultState)
{
    const uint8_t pin = 5;
    EXPECT_CALL(mock_gpio, set_mode(pin, hal::GpioMode::MODE_OUTPUT)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin, hal::GpioLevel::LEVEL_LOW)).Times(1);

    hal::DigitalOutput valve(mock_gpio, pin, false, false);
    valve.init();

    EXPECT_FALSE(valve.is_on());
    EXPECT_EQ(valve.get_pin(), pin);
}

TEST_F(DigitalOutputTest, TurnsOnAndOffActiveHigh)
{
    const uint8_t pin = 6;
    hal::DigitalOutput pump(mock_gpio, pin, false, false);
    pump.init();

    EXPECT_CALL(mock_gpio, set_level(pin, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    pump.turn_on();
    EXPECT_TRUE(pump.is_on());

    EXPECT_CALL(mock_gpio, set_level(pin, hal::GpioLevel::LEVEL_LOW)).Times(1);
    pump.turn_off();
    EXPECT_FALSE(pump.is_on());
}

TEST_F(DigitalOutputTest, SupportsActiveLowRelays)
{
    const uint8_t pin = 7;
    // active_low = true: ON -> LEVEL_LOW, OFF -> LEVEL_HIGH
    hal::DigitalOutput relay(mock_gpio, pin, true, false);
    relay.init();

    // Turn ON sends LEVEL_LOW to active-low relay
    EXPECT_CALL(mock_gpio, set_level(pin, hal::GpioLevel::LEVEL_LOW)).Times(1);
    relay.turn_on();
    EXPECT_TRUE(relay.is_on());

    // Turn OFF sends LEVEL_HIGH to active-low relay
    EXPECT_CALL(mock_gpio, set_level(pin, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    relay.turn_off();
    EXPECT_FALSE(relay.is_on());
}

TEST_F(DigitalOutputTest, TogglesOutputState)
{
    const uint8_t pin = 8;
    hal::DigitalOutput buzzer(mock_gpio, pin, false, false);
    buzzer.init();

    EXPECT_CALL(mock_gpio, set_level(pin, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    buzzer.toggle();
    EXPECT_TRUE(buzzer.is_on());

    EXPECT_CALL(mock_gpio, set_level(pin, hal::GpioLevel::LEVEL_LOW)).Times(1);
    buzzer.toggle();
    EXPECT_FALSE(buzzer.is_on());
}

TEST_F(DigitalOutputTest, BatchOperationsViaIntrusiveLinkedList)
{
    const uint8_t pin1 = 9;
    const uint8_t pin2 = 10;
    const uint8_t pin3 = 11;

    hal::DigitalOutput out1(mock_gpio, pin1, false, false);
    hal::DigitalOutput out2(mock_gpio, pin2, false, false);
    hal::DigitalOutput out3(mock_gpio, pin3, false, false);

    // Batch initialization
    EXPECT_CALL(mock_gpio, set_mode(pin1, hal::GpioMode::MODE_OUTPUT)).Times(1);
    EXPECT_CALL(mock_gpio, set_mode(pin2, hal::GpioMode::MODE_OUTPUT)).Times(1);
    EXPECT_CALL(mock_gpio, set_mode(pin3, hal::GpioMode::MODE_OUTPUT)).Times(1);
    hal::DigitalOutput::init_all();

    out1.turn_on();
    out2.turn_on();
    out3.turn_on();
    EXPECT_TRUE(out1.is_on());
    EXPECT_TRUE(out2.is_on());
    EXPECT_TRUE(out3.is_on());

    // Emergency batch turn off all
    hal::DigitalOutput::turn_off_all();
    EXPECT_FALSE(out1.is_on());
    EXPECT_FALSE(out2.is_on());
    EXPECT_FALSE(out3.is_on());
}
