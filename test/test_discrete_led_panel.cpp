#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_gpio_hal.hpp"
#include "mocks/mock_timer_hal.hpp"
#include "ui/discrete_led_panel.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::InSequence;

class DiscreteLedPanelTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockGpioHAL> mock_gpio;
    NiceMock<mocks::MockTimerHAL> mock_timer;

    const uint8_t pin_power = 7;
    const uint8_t pin_softener = 6;
    const uint8_t pin_wash = 16;
    const uint8_t pin_rinse = 15;
    const uint8_t pin_spin = 14;
    const uint8_t pin_lvl_low = 13;
    const uint8_t pin_lvl_med = 12;

    ui::DiscreteLedPins pins{
        pin_power, pin_softener, pin_wash, pin_rinse, pin_spin,
        pin_lvl_low, pin_lvl_med, 255
    };

    ui::DiscreteLedPanel panel{mock_gpio, mock_timer, pins, true};

    void SetUp() override
    {
        panel.init();
    }
};

TEST_F(DiscreteLedPanelTest, InitializesAllPinsToOutputAndOff)
{
    EXPECT_CALL(mock_gpio, set_mode(pin_power, hal::GpioMode::MODE_OUTPUT)).Times(1);
    EXPECT_CALL(mock_gpio, set_mode(pin_softener, hal::GpioMode::MODE_OUTPUT)).Times(1);
    EXPECT_CALL(mock_gpio, set_mode(pin_wash, hal::GpioMode::MODE_OUTPUT)).Times(1);
    EXPECT_CALL(mock_gpio, set_mode(pin_rinse, hal::GpioMode::MODE_OUTPUT)).Times(1);
    EXPECT_CALL(mock_gpio, set_mode(pin_spin, hal::GpioMode::MODE_OUTPUT)).Times(1);
    EXPECT_CALL(mock_gpio, set_mode(pin_lvl_low, hal::GpioMode::MODE_OUTPUT)).Times(1);
    EXPECT_CALL(mock_gpio, set_mode(pin_lvl_med, hal::GpioMode::MODE_OUTPUT)).Times(1);

    ui::DiscreteLedPanel fresh_panel{mock_gpio, mock_timer, pins, true};
    fresh_panel.init();
}

TEST_F(DiscreteLedPanelTest, ControlsPowerAndSoftenerIndicators)
{
    EXPECT_CALL(mock_gpio, set_level(pin_power, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    panel.set_power(true);

    EXPECT_CALL(mock_gpio, set_level(pin_softener, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    panel.set_softener(true);

    EXPECT_CALL(mock_gpio, set_level(pin_power, hal::GpioLevel::LEVEL_LOW)).Times(1);
    panel.set_power(false);
}

TEST_F(DiscreteLedPanelTest, SetsProgramsDuringIdleSelection)
{
    // NORMAL_WASH: wash=HIGH (solid), rinse=LOW, spin=LOW
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_rinse, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_spin, hal::GpioLevel::LEVEL_LOW)).Times(1);
    panel.set_program(ui::WashProgram::NORMAL_WASH);

    // RINSE_ONLY: wash=LOW, rinse=HIGH, spin=LOW
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_rinse, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_spin, hal::GpioLevel::LEVEL_LOW)).Times(1);
    panel.set_program(ui::WashProgram::RINSE_ONLY);

    // SPIN_ONLY: wash=LOW, rinse=LOW, spin=HIGH
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_rinse, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_spin, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    panel.set_program(ui::WashProgram::SPIN_ONLY);
}

TEST_F(DiscreteLedPanelTest, HeavyWashProgramBlinksWashLedNonBlocking)
{
    EXPECT_CALL(mock_timer, get_time_ms()).WillRepeatedly(Return(1000));
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_rinse, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_spin, hal::GpioLevel::LEVEL_LOW)).Times(1);
    panel.set_program(ui::WashProgram::HEAVY_WASH);

    // Update before 500ms -> no toggle
    EXPECT_CALL(mock_timer, get_time_ms()).WillRepeatedly(Return(1200));
    panel.update();

    // Update after 500ms -> toggles to LOW
    EXPECT_CALL(mock_timer, get_time_ms()).WillRepeatedly(Return(1500));
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_LOW)).Times(1);
    panel.update();

    // Update after another 500ms -> toggles back to HIGH
    EXPECT_CALL(mock_timer, get_time_ms()).WillRepeatedly(Return(2000));
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    panel.update();
}

TEST_F(DiscreteLedPanelTest, SetsWashStagesExclusively)
{
    // Stage WASH: wash=HIGH, rinse=LOW, spin=LOW
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_rinse, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_spin, hal::GpioLevel::LEVEL_LOW)).Times(1);
    panel.set_stage(ui::WashStage::WASH);

    // Stage RINSE: wash=LOW, rinse=HIGH, spin=LOW
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_rinse, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_spin, hal::GpioLevel::LEVEL_LOW)).Times(1);
    panel.set_stage(ui::WashStage::RINSE);

    // Stage SPIN: wash=LOW, rinse=LOW, spin=HIGH
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_rinse, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_spin, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    panel.set_stage(ui::WashStage::SPIN);
}

TEST_F(DiscreteLedPanelTest, ControlsWaterLevelIndicators)
{
    // LOW_LEVEL: low=HIGH, med=LOW
    EXPECT_CALL(mock_gpio, set_level(pin_lvl_low, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_lvl_med, hal::GpioLevel::LEVEL_LOW)).Times(1);
    panel.set_selected_level(hal::WaterLevel::LOW_LEVEL);

    // MEDIUM_LEVEL: low=LOW, med=HIGH
    EXPECT_CALL(mock_gpio, set_level(pin_lvl_low, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_lvl_med, hal::GpioLevel::LEVEL_HIGH)).Times(1);
    panel.set_selected_level(hal::WaterLevel::MEDIUM_LEVEL);
}

TEST_F(DiscreteLedPanelTest, TurnsOffAllLedsOnPanel)
{
    EXPECT_CALL(mock_gpio, set_level(pin_power, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_softener, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_wash, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_rinse, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_spin, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_lvl_low, hal::GpioLevel::LEVEL_LOW)).Times(1);
    EXPECT_CALL(mock_gpio, set_level(pin_lvl_med, hal::GpioLevel::LEVEL_LOW)).Times(1);

    panel.turn_off_all();
}
