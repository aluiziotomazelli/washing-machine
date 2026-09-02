#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ui/strip_led_panel.hpp"
#include "hal/ws2812_strip.hpp"
#include "mocks/mock_timer_hal.hpp"

using namespace testing;
using namespace ui;
using namespace domain;
using namespace hal;

class StripLedPanelTest : public Test {
protected:
    Ws2812Strip strip{6, 9}; // 9 pixels buffer
    NiceMock<mocks::MockTimerHAL> mock_timer;

    StripLedConfig config{
        100, // max_brightness = 100
        80,  // idle_brightness = 80
        20,  // future_brightness = 20
        2000,// breathe_period_ms = 2000
        500  // blink_interval_ms = 500
    };

    StripLedPanel panel{strip, mock_timer, config};

    void SetUp() override {
        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Return(1000));
    }
};

TEST_F(StripLedPanelTest, InitializesPanelAndRendersDefaultState)
{
    panel.init();

    // Default program is RINSE_ONLY, level LOW_LEVEL, softener false, IDLE
    // P0 (softener) = OFF
    EXPECT_EQ(strip.get_pixel(0), RgbColor(0, 0, 0));
    // P1 (gap) = OFF
    EXPECT_EQ(strip.get_pixel(1), RgbColor(0, 0, 0));
    // P2 (level low) = CYAN
    RgbColor p2 = strip.get_pixel(2);
    EXPECT_GT(p2.g, 0);
    EXPECT_GT(p2.b, 0);
    EXPECT_EQ(p2.r, 0);
    // P3, P4 = OFF
    EXPECT_EQ(strip.get_pixel(3), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(4), RgbColor(0, 0, 0));
    // P5, P6 = OFF (Rinse only)
    EXPECT_EQ(strip.get_pixel(5), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(6), RgbColor(0, 0, 0));
    // P7 (rinse), P8 (spin) = WHITE
    RgbColor p7 = strip.get_pixel(7);
    EXPECT_GT(p7.r, 0);
    EXPECT_GT(p7.g, 0);
    EXPECT_GT(p7.b, 0);
    RgbColor p8 = strip.get_pixel(8);
    EXPECT_GT(p8.r, 0);
    EXPECT_GT(p8.g, 0);
    EXPECT_GT(p8.b, 0);
}

TEST_F(StripLedPanelTest, RendersIdleNormalWashSelectionInWhite)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);

    // Normal wash: P6 (L), P7 (E), P8 (C) in White. P5 (Heavy) is OFF.
    EXPECT_EQ(strip.get_pixel(5), RgbColor(0, 0, 0));

    RgbColor p6 = strip.get_pixel(6);
    RgbColor p7 = strip.get_pixel(7);
    RgbColor p8 = strip.get_pixel(8);

    EXPECT_GT(p6.r, 0);
    EXPECT_GT(p6.g, 0);
    EXPECT_GT(p6.b, 0);

    EXPECT_GT(p7.r, 0);
    EXPECT_GT(p7.g, 0);
    EXPECT_GT(p7.b, 0);

    EXPECT_GT(p8.r, 0);
    EXPECT_GT(p8.g, 0);
    EXPECT_GT(p8.b, 0);
}

TEST_F(StripLedPanelTest, RendersIdleHeavyWashSelectionWithPixel5And6InWhite)
{
    panel.init();
    panel.set_program(WashProgram::HEAVY_WASH);

    // Heavy wash: P5 (Heavy/Soak) AND P6 (Wash), P7 (Rinse), P8 (Spin) all lit in White!
    RgbColor p5 = strip.get_pixel(5);
    RgbColor p6 = strip.get_pixel(6);
    RgbColor p7 = strip.get_pixel(7);
    RgbColor p8 = strip.get_pixel(8);

    EXPECT_GT(p5.r, 0);
    EXPECT_GT(p5.g, 0);
    EXPECT_GT(p5.b, 0);

    EXPECT_GT(p6.r, 0);
    EXPECT_GT(p6.g, 0);
    EXPECT_GT(p6.b, 0);

    EXPECT_GT(p7.r, 0);
    EXPECT_GT(p8.r, 0);
}

TEST_F(StripLedPanelTest, RendersIdleSpinOnlySelection)
{
    panel.init();
    panel.set_program(WashProgram::SPIN_ONLY);

    EXPECT_EQ(strip.get_pixel(5), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(6), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(7), RgbColor(0, 0, 0));

    // Only P8 is ON
    RgbColor p8 = strip.get_pixel(8);
    EXPECT_GT(p8.r, 0);
    EXPECT_GT(p8.g, 0);
    EXPECT_GT(p8.b, 0);
}

TEST_F(StripLedPanelTest, RendersSoftenerActivePink)
{
    panel.init();
    panel.set_softener(true);

    RgbColor p0 = strip.get_pixel(0);
    // Pink: R > 0, B > 0, R > G
    EXPECT_GT(p0.r, 0);
    EXPECT_GT(p0.b, 0);
    EXPECT_GT(p0.r, p0.g);
}

TEST_F(StripLedPanelTest, RendersMediumAndHighWaterLevels)
{
    panel.init();

    // Medium level: P2 and P3 are Cyan, P4 is OFF
    panel.set_selected_level(WaterLevel::MEDIUM_LEVEL);
    EXPECT_GT(strip.get_pixel(2).b, 0);
    EXPECT_GT(strip.get_pixel(3).b, 0);
    EXPECT_EQ(strip.get_pixel(4), RgbColor(0, 0, 0));

    // High level: P2, P3, P4 are Cyan
    panel.set_selected_level(WaterLevel::HIGH_LEVEL);
    EXPECT_GT(strip.get_pixel(2).b, 0);
    EXPECT_GT(strip.get_pixel(3).b, 0);
    EXPECT_GT(strip.get_pixel(4).b, 0);
}

TEST_F(StripLedPanelTest, RendersRunningStateWithBreathingWashAndFaintFutureStages)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);
    panel.set_machine_state(MachineState::RUNNING);
    panel.set_stage(WashStage::WASH);

    // Wash active: P6 breathes (bright), P7 and P8 are faint (dim)
    RgbColor p6 = strip.get_pixel(6);
    RgbColor p7 = strip.get_pixel(7);
    RgbColor p8 = strip.get_pixel(8);

    EXPECT_GT(p6.r, p7.r);
    EXPECT_GT(p6.r, p8.r);
    EXPECT_GT(p7.r, 0);
    EXPECT_GT(p8.r, 0);
}

TEST_F(StripLedPanelTest, RendersRunningRinseStageWithWashOffAndSpinFaint)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);
    panel.set_machine_state(MachineState::RUNNING);
    panel.set_stage(WashStage::RINSE);

    // Wash finished (OFF), Rinse active (breathing bright), Spin future (faint)
    EXPECT_EQ(strip.get_pixel(6), RgbColor(0, 0, 0));

    RgbColor p7 = strip.get_pixel(7);
    RgbColor p8 = strip.get_pixel(8);

    EXPECT_GT(p7.r, p8.r);
    EXPECT_GT(p8.r, 0);
}

TEST_F(StripLedPanelTest, RendersRunningSpinStageWithWashAndRinseOff)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);
    panel.set_machine_state(MachineState::RUNNING);
    panel.set_stage(WashStage::SPIN);

    EXPECT_EQ(strip.get_pixel(6), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(7), RgbColor(0, 0, 0));

    RgbColor p8 = strip.get_pixel(8);
    EXPECT_GT(p8.r, 0);
}

TEST_F(StripLedPanelTest, RendersPausedStateBlinkingBetweenOnAndOff)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);
    panel.set_machine_state(MachineState::PAUSED);
    panel.set_stage(WashStage::WASH);

    // Phase 1 (0ms): blink_state_ is true -> pixels are lit
    EXPECT_GT(strip.get_pixel(6).r, 0);

    // Phase 2 (550ms): blink_state_ is false -> pixels are OFF
    ON_CALL(mock_timer, get_time_ms()).WillByDefault(Return(1550));
    panel.update();
    EXPECT_EQ(strip.get_pixel(6), RgbColor(0, 0, 0));

    // Phase 3 (1050ms): blink_state_ is true -> pixels are lit again
    ON_CALL(mock_timer, get_time_ms()).WillByDefault(Return(2050));
    panel.update();
    EXPECT_GT(strip.get_pixel(6).r, 0);
}

TEST_F(StripLedPanelTest, RendersFillTimeoutErrorBlinkingRedWaterLevels)
{
    panel.init();
    panel.set_machine_state(MachineState::ERROR, MachineError::FILL_TIMEOUT);

    // Error on Water Level: P2, P3, P4 blink in RED, programs are OFF
    RgbColor p2 = strip.get_pixel(2);
    EXPECT_GT(p2.r, 0);
    EXPECT_EQ(p2.g, 0);
    EXPECT_EQ(p2.b, 0);

    EXPECT_EQ(strip.get_pixel(6), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(7), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(8), RgbColor(0, 0, 0));
}

TEST_F(StripLedPanelTest, RendersDrainTimeoutErrorBlinkingRedProgramStages)
{
    panel.init();
    panel.set_machine_state(MachineState::ERROR, MachineError::DRAIN_TIMEOUT);

    // Error on Drain: P6, P7, P8 blink in RED, levels are OFF
    RgbColor p6 = strip.get_pixel(6);
    EXPECT_GT(p6.r, 0);
    EXPECT_EQ(p6.g, 0);
    EXPECT_EQ(p6.b, 0);

    EXPECT_EQ(strip.get_pixel(2), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(3), RgbColor(0, 0, 0));
}

TEST_F(StripLedPanelTest, TurnOffAllClearsAllPixelsAndCallsShow)
{
    panel.init();
    panel.set_softener(true);
    panel.set_program(WashProgram::HEAVY_WASH);

    panel.turn_off_all();

    for (uint8_t i = 0; i < 9; ++i) {
        EXPECT_EQ(strip.get_pixel(i), RgbColor(0, 0, 0));
    }
}
