#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_gpio_hal.hpp"
#include "mocks/mock_timer_hal.hpp"
#include "ui/button.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::Invoke;

class ButtonTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockGpioHAL> mock_gpio;
    NiceMock<mocks::MockTimerHAL> mock_timer;
    const uint8_t pin = 2;
    ui::Button btn{mock_gpio, mock_timer, pin};

    hal::GpioLevel simulated_level{hal::GpioLevel::LEVEL_HIGH};
    uint32_t simulated_time_ms{0};

    void SetUp() override
    {
        simulated_level = hal::GpioLevel::LEVEL_HIGH;
        simulated_time_ms = 0;

        // Default GMock actions linking to simulated state
        ON_CALL(mock_gpio, get_level(pin)).WillByDefault(Invoke([this](uint8_t) {
            return simulated_level;
        }));

        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Invoke([this]() {
            return simulated_time_ms;
        }));

        EXPECT_CALL(mock_gpio, set_mode(pin, hal::GpioMode::MODE_INPUT_PULLUP)).Times(AtLeast(1));
        btn.init();
        btn.update();
    }

    void advance_time_ms(uint32_t ms)
    {
        simulated_time_ms += ms;
    }

    void run_for(uint32_t total_ms, uint32_t step_ms = 5)
    {
        uint32_t elapsed = 0;
        while (elapsed < total_ms) {
            advance_time_ms(step_ms);
            btn.update();
            elapsed += step_ms;
        }
    }

    void press_button()
    {
        simulated_level = hal::GpioLevel::LEVEL_LOW;
        btn.update();
    }

    void release_button()
    {
        simulated_level = hal::GpioLevel::LEVEL_HIGH;
        btn.update();
    }
};

TEST_F(ButtonTest, InitializesWithInputPullupByDefault)
{
    EXPECT_FALSE(btn.is_pressed());
    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::NONE_CLICK);
}

TEST_F(ButtonTest, DetectsSingleClickAfterDebounceAndRelease)
{
    // 1. Press and hold for 100 ms (past 20ms debounce)
    press_button();
    run_for(100);

    EXPECT_TRUE(btn.is_pressed());
    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::NONE_CLICK);

    // 2. Release and wait past double-click window (> 300ms)
    release_button();
    run_for(350);

    EXPECT_FALSE(btn.is_pressed());
    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::CLICK);

    // Reading again must be consumed
    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::NONE_CLICK);
}

TEST_F(ButtonTest, IgnoresElectricalNoiseGlitchBelowDebounceThreshold)
{
    // Glitch pulse for only 10 ms (below 20ms debounce)
    press_button();
    run_for(10);

    release_button();
    run_for(500);

    EXPECT_FALSE(btn.is_pressed());
    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::NONE_CLICK);
}

TEST_F(ButtonTest, DetectsDoubleClick)
{
    // First Click
    press_button();
    run_for(50);
    release_button();
    run_for(50);

    // Second Click within 300ms window
    press_button();
    run_for(50);
    release_button();
    run_for(350); // Wait for double click window to close

    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::DOUBLE_CLICK);
}

TEST_F(ButtonTest, DetectsLongClickForStageSkip)
{
    // Press and hold for 1500 ms (long click threshold is 1000ms)
    press_button();
    run_for(1500);

    EXPECT_TRUE(btn.is_pressed());

    // Release button
    release_button();
    run_for(50);

    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::LONG_CLICK);
}

TEST_F(ButtonTest, DetectsVeryLongClick)
{
    // Press and hold for 3500 ms (very long click threshold is 3000ms)
    press_button();
    run_for(3500);

    release_button();
    run_for(50);

    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::VERY_LONG_CLICK);
}

TEST_F(ButtonTest, DetectsStuckButtonTimeout)
{
    // Hold button continuously past timeout_ms (6000ms)
    press_button();
    run_for(6500);

    // Release after timeout
    release_button();
    run_for(50);

    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::TIMEOUT);
}

TEST_F(ButtonTest, DetectsPersistentStuckButtonErrorState)
{
    // Hold button continuously past 2 * timeout_ms (12000ms)
    press_button();
    run_for(13000);

    EXPECT_EQ(btn.get_last_click(), ui::ButtonClickType::ERROR_STATE);
}
