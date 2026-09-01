#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_gpio_hal.hpp"
#include "mocks/mock_timer_hal.hpp"
#include "hal/pressure_switch_sensor.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::Invoke;

class PressureSwitchSensorTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockGpioHAL> mock_gpio;
    NiceMock<mocks::MockTimerHAL> mock_timer;

    const uint8_t pin_low = 10;
    const uint8_t pin_med = 11;
    const uint8_t pin_high = 12;

    hal::PressureSwitchConfig config{
        {pin_low, hal::ContactType::NORMALLY_CLOSED},  // NC
        {pin_med, hal::ContactType::NORMALLY_OPEN},    // NO
        {pin_high, hal::ContactType::NORMALLY_OPEN},   // NO
        100 // 100 ms debounce
    };

    hal::PressureSwitchSensor sensor{mock_gpio, mock_timer, config};

    // Physical pin levels in empty tub:
    // NC contact 31-32 closed to GND -> LEVEL_LOW
    // NO contact 11-13 open with pull-up -> LEVEL_HIGH
    // NO contact 21-23 open with pull-up -> LEVEL_HIGH
    hal::GpioLevel level_low_pin{hal::GpioLevel::LEVEL_LOW};
    hal::GpioLevel level_med_pin{hal::GpioLevel::LEVEL_HIGH};
    hal::GpioLevel level_high_pin{hal::GpioLevel::LEVEL_HIGH};

    uint32_t simulated_time_ms{0};

    void SetUp() override
    {
        simulated_time_ms = 0;
        level_low_pin = hal::GpioLevel::LEVEL_LOW;
        level_med_pin = hal::GpioLevel::LEVEL_HIGH;
        level_high_pin = hal::GpioLevel::LEVEL_HIGH;

        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Invoke([this]() {
            return simulated_time_ms;
        }));

        ON_CALL(mock_gpio, get_level(pin_low)).WillByDefault(Invoke([this](uint8_t) {
            return level_low_pin;
        }));
        ON_CALL(mock_gpio, get_level(pin_med)).WillByDefault(Invoke([this](uint8_t) {
            return level_med_pin;
        }));
        ON_CALL(mock_gpio, get_level(pin_high)).WillByDefault(Invoke([this](uint8_t) {
            return level_high_pin;
        }));

        sensor.init();
        sensor.update();
    }

    void run_for(uint32_t total_ms, uint32_t step_ms = 10)
    {
        uint32_t elapsed = 0;
        while (elapsed < total_ms) {
            simulated_time_ms += step_ms;
            sensor.update();
            elapsed += step_ms;
        }
    }
};

TEST_F(PressureSwitchSensorTest, ReportsEmptyTubInitially)
{
    EXPECT_TRUE(sensor.is_empty());
    EXPECT_EQ(sensor.get_current_level(), hal::WaterLevel::EMPTY);
    EXPECT_FALSE(sensor.is_level_reached(hal::WaterLevel::LOW_LEVEL));
}

TEST_F(PressureSwitchSensorTest, DetectsLowLevelAfterDebounce)
{
    // Water pressure rises: NC contact opens (pin goes HIGH)
    level_low_pin = hal::GpioLevel::LEVEL_HIGH;
    run_for(50); // In debounce window (< 100ms)

    EXPECT_TRUE(sensor.is_empty());

    // Advance past 100ms debounce threshold (run 100ms more)
    run_for(100);

    EXPECT_FALSE(sensor.is_empty());
    EXPECT_TRUE(sensor.is_level_reached(hal::WaterLevel::LOW_LEVEL));
    EXPECT_FALSE(sensor.is_level_reached(hal::WaterLevel::MEDIUM_LEVEL));
    EXPECT_EQ(sensor.get_current_level(), hal::WaterLevel::LOW_LEVEL);
}

TEST_F(PressureSwitchSensorTest, DetectsMediumLevelAndImpliesLowLevel)
{
    // Both Low (NC open -> HIGH) and Medium (NO closed -> LOW) are active
    level_low_pin = hal::GpioLevel::LEVEL_HIGH;
    level_med_pin = hal::GpioLevel::LEVEL_LOW;
    run_for(150);

    EXPECT_FALSE(sensor.is_empty());
    EXPECT_TRUE(sensor.is_level_reached(hal::WaterLevel::LOW_LEVEL));
    EXPECT_TRUE(sensor.is_level_reached(hal::WaterLevel::MEDIUM_LEVEL));
    EXPECT_FALSE(sensor.is_level_reached(hal::WaterLevel::HIGH_LEVEL));
    EXPECT_EQ(sensor.get_current_level(), hal::WaterLevel::MEDIUM_LEVEL);
}

TEST_F(PressureSwitchSensorTest, DetectsHighLevel)
{
    level_low_pin = hal::GpioLevel::LEVEL_HIGH;
    level_med_pin = hal::GpioLevel::LEVEL_LOW;
    level_high_pin = hal::GpioLevel::LEVEL_LOW; // High NO closes -> LOW
    run_for(150);

    EXPECT_FALSE(sensor.is_empty());
    EXPECT_TRUE(sensor.is_level_reached(hal::WaterLevel::LOW_LEVEL));
    EXPECT_TRUE(sensor.is_level_reached(hal::WaterLevel::MEDIUM_LEVEL));
    EXPECT_TRUE(sensor.is_level_reached(hal::WaterLevel::HIGH_LEVEL));
    EXPECT_EQ(sensor.get_current_level(), hal::WaterLevel::HIGH_LEVEL);
}

TEST_F(PressureSwitchSensorTest, IgnoresHydraulicSloshingGlitches)
{
    // Transient wave pulse for only 30ms (< 100ms debounce)
    level_low_pin = hal::GpioLevel::LEVEL_HIGH;
    run_for(30);

    // Wave recedes back to LOW
    level_low_pin = hal::GpioLevel::LEVEL_LOW;
    run_for(150);

    EXPECT_TRUE(sensor.is_empty());
    EXPECT_FALSE(sensor.is_level_reached(hal::WaterLevel::LOW_LEVEL));
}
