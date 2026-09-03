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
        500, // blink_interval_ms = 500
        20   // frame_interval_ms = 20 (50 FPS)
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
    // Softener = OFF
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_softener), RgbColor(0, 0, 0));
    // Gap = OFF
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_gap1), RgbColor(0, 0, 0));
    // Level low = CYAN
    RgbColor p_low = strip.get_pixel(StripLedPanel::k_idx_lvl_low);
    EXPECT_GT(p_low.g, 0);
    EXPECT_GT(p_low.b, 0);
    EXPECT_EQ(p_low.r, 0);
    // Level med, high = OFF
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_lvl_med), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_lvl_high), RgbColor(0, 0, 0));
    // Heavy, Wash = OFF (Rinse only)
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_heavy_wash), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_wash), RgbColor(0, 0, 0));
    // Rinse, Spin = WHITE
    RgbColor p_rinse = strip.get_pixel(StripLedPanel::k_idx_rinse);
    EXPECT_GT(p_rinse.r, 0);
    EXPECT_GT(p_rinse.g, 0);
    EXPECT_GT(p_rinse.b, 0);
    RgbColor p_spin = strip.get_pixel(StripLedPanel::k_idx_spin);
    EXPECT_GT(p_spin.r, 0);
    EXPECT_GT(p_spin.g, 0);
    EXPECT_GT(p_spin.b, 0);
}

TEST_F(StripLedPanelTest, RendersIdleNormalWashSelectionInWhite)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);

    // Normal wash: Wash (L), Rinse (E), Spin (C) in White. Heavy is OFF.
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_heavy_wash), RgbColor(0, 0, 0));

    RgbColor p_wash = strip.get_pixel(StripLedPanel::k_idx_wash);
    RgbColor p_rinse = strip.get_pixel(StripLedPanel::k_idx_rinse);
    RgbColor p_spin = strip.get_pixel(StripLedPanel::k_idx_spin);

    EXPECT_GT(p_wash.r, 0);
    EXPECT_GT(p_wash.g, 0);
    EXPECT_GT(p_wash.b, 0);

    EXPECT_GT(p_rinse.r, 0);
    EXPECT_GT(p_rinse.g, 0);
    EXPECT_GT(p_rinse.b, 0);

    EXPECT_GT(p_spin.r, 0);
    EXPECT_GT(p_spin.g, 0);
    EXPECT_GT(p_spin.b, 0);
}

TEST_F(StripLedPanelTest, RendersIdleHeavyWashSelectionWithPixel5And6InWhite)
{
    panel.init();
    panel.set_program(WashProgram::HEAVY_WASH);

    // Heavy wash: Heavy/Soak AND Wash, Rinse, Spin all lit in White!
    RgbColor p_heavy = strip.get_pixel(StripLedPanel::k_idx_heavy_wash);
    RgbColor p_wash = strip.get_pixel(StripLedPanel::k_idx_wash);
    RgbColor p_rinse = strip.get_pixel(StripLedPanel::k_idx_rinse);
    RgbColor p_spin = strip.get_pixel(StripLedPanel::k_idx_spin);

    EXPECT_GT(p_heavy.r, 0);
    EXPECT_GT(p_heavy.g, 0);
    EXPECT_GT(p_heavy.b, 0);

    EXPECT_GT(p_wash.r, 0);
    EXPECT_GT(p_wash.g, 0);
    EXPECT_GT(p_wash.b, 0);

    EXPECT_GT(p_rinse.r, 0);
    EXPECT_GT(p_spin.r, 0);
}

TEST_F(StripLedPanelTest, RendersIdleSpinOnlySelection)
{
    panel.init();
    panel.set_program(WashProgram::SPIN_ONLY);

    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_heavy_wash), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_wash), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_rinse), RgbColor(0, 0, 0));

    // Only Spin is ON
    RgbColor p_spin = strip.get_pixel(StripLedPanel::k_idx_spin);
    EXPECT_GT(p_spin.r, 0);
    EXPECT_GT(p_spin.g, 0);
    EXPECT_GT(p_spin.b, 0);
}

TEST_F(StripLedPanelTest, RendersSoftenerActivePink)
{
    panel.init();
    panel.set_softener(true);

    RgbColor p_softener = strip.get_pixel(StripLedPanel::k_idx_softener);
    // Pink: R > 0, B > 0, R > G
    EXPECT_GT(p_softener.r, 0);
    EXPECT_GT(p_softener.b, 0);
    EXPECT_GT(p_softener.r, p_softener.g);
}

TEST_F(StripLedPanelTest, RendersMediumAndHighWaterLevels)
{
    panel.init();

    // Medium level: Low and Med are Cyan, High is OFF
    panel.set_selected_level(WaterLevel::MEDIUM_LEVEL);
    EXPECT_GT(strip.get_pixel(StripLedPanel::k_idx_lvl_low).b, 0);
    EXPECT_GT(strip.get_pixel(StripLedPanel::k_idx_lvl_med).b, 0);
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_lvl_high), RgbColor(0, 0, 0));

    // High level: Low, Med, High are Cyan
    panel.set_selected_level(WaterLevel::HIGH_LEVEL);
    EXPECT_GT(strip.get_pixel(StripLedPanel::k_idx_lvl_low).b, 0);
    EXPECT_GT(strip.get_pixel(StripLedPanel::k_idx_lvl_med).b, 0);
    EXPECT_GT(strip.get_pixel(StripLedPanel::k_idx_lvl_high).b, 0);
}

TEST_F(StripLedPanelTest, RendersRunningStateWithBreathingWashAndFaintFutureStages)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);
    panel.set_machine_state(MachineState::RUNNING);
    panel.set_stage(WashStage::WASH);

    // Wash active: Wash breathes (bright), Rinse and Spin are faint (dim)
    RgbColor p_wash = strip.get_pixel(StripLedPanel::k_idx_wash);
    RgbColor p_rinse = strip.get_pixel(StripLedPanel::k_idx_rinse);
    RgbColor p_spin = strip.get_pixel(StripLedPanel::k_idx_spin);

    EXPECT_GT(p_wash.r, p_rinse.r);
    EXPECT_GT(p_wash.r, p_spin.r);
    EXPECT_GT(p_rinse.r, 0);
    EXPECT_GT(p_spin.r, 0);
}

TEST_F(StripLedPanelTest, RendersRunningRinseStageWithWashOffAndSpinFaint)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);
    panel.set_machine_state(MachineState::RUNNING);
    panel.set_stage(WashStage::RINSE);

    // Wash finished (OFF), Rinse active (breathing bright), Spin future (faint)
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_wash), RgbColor(0, 0, 0));

    RgbColor p_rinse = strip.get_pixel(StripLedPanel::k_idx_rinse);
    RgbColor p_spin = strip.get_pixel(StripLedPanel::k_idx_spin);

    EXPECT_GT(p_rinse.r, p_spin.r);
    EXPECT_GT(p_spin.r, 0);
}

TEST_F(StripLedPanelTest, RendersRunningSpinStageWithWashAndRinseOff)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);
    panel.set_machine_state(MachineState::RUNNING);
    panel.set_stage(WashStage::SPIN);

    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_wash), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_rinse), RgbColor(0, 0, 0));

    RgbColor p_spin = strip.get_pixel(StripLedPanel::k_idx_spin);
    EXPECT_GT(p_spin.r, 0);
}

TEST_F(StripLedPanelTest, RendersPausedStateBlinkingBetweenOnAndOff)
{
    panel.init();
    panel.set_program(WashProgram::NORMAL_WASH);
    panel.set_machine_state(MachineState::PAUSED);
    panel.set_stage(WashStage::WASH);

    // Phase 1 (0ms): blink_state_ is true -> pixels are lit
    EXPECT_GT(strip.get_pixel(StripLedPanel::k_idx_wash).r, 0);

    // Phase 2 (550ms): blink_state_ is false -> pixels are OFF
    ON_CALL(mock_timer, get_time_ms()).WillByDefault(Return(1550));
    panel.update();
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_wash), RgbColor(0, 0, 0));

    // Phase 3 (1050ms): blink_state_ is true -> pixels are lit again
    ON_CALL(mock_timer, get_time_ms()).WillByDefault(Return(2050));
    panel.update();
    EXPECT_GT(strip.get_pixel(StripLedPanel::k_idx_wash).r, 0);
}

TEST_F(StripLedPanelTest, RendersFillTimeoutErrorBlinkingRedWaterLevels)
{
    panel.init();
    panel.set_machine_state(MachineState::ERROR, MachineError::FILL_TIMEOUT);

    // Error on Water Level: Low, Med, High blink in RED, programs are OFF
    RgbColor p_low = strip.get_pixel(StripLedPanel::k_idx_lvl_low);
    EXPECT_GT(p_low.r, 0);
    EXPECT_EQ(p_low.g, 0);
    EXPECT_EQ(p_low.b, 0);

    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_wash), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_rinse), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_spin), RgbColor(0, 0, 0));
}

TEST_F(StripLedPanelTest, RendersDrainTimeoutErrorBlinkingRedProgramStages)
{
    panel.init();
    panel.set_machine_state(MachineState::ERROR, MachineError::DRAIN_TIMEOUT);

    // Error on Drain: Wash, Rinse, Spin blink in RED, levels are OFF
    RgbColor p_wash = strip.get_pixel(StripLedPanel::k_idx_wash);
    EXPECT_GT(p_wash.r, 0);
    EXPECT_EQ(p_wash.g, 0);
    EXPECT_EQ(p_wash.b, 0);

    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_lvl_low), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_lvl_med), RgbColor(0, 0, 0));
}

TEST_F(StripLedPanelTest, RendersUnbalancedLoadErrorBlinkingRedSpin)
{
    panel.init();
    panel.set_machine_state(MachineState::ERROR, MachineError::UNBALANCED_LOAD);

    // Error on Unbalance: Spin LED blinks in RED, all other LEDs are OFF
    RgbColor p_spin = strip.get_pixel(StripLedPanel::k_idx_spin);
    EXPECT_GT(p_spin.r, 0);
    EXPECT_EQ(p_spin.g, 0);
    EXPECT_EQ(p_spin.b, 0);

    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_wash), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_rinse), RgbColor(0, 0, 0));
    EXPECT_EQ(strip.get_pixel(StripLedPanel::k_idx_lvl_low), RgbColor(0, 0, 0));
}

TEST_F(StripLedPanelTest, TurnOffAllClearsAllPixelsAndCallsShow)
{
    panel.init();
    panel.set_softener(true);
    panel.set_program(WashProgram::HEAVY_WASH);

    panel.turn_off_all();

    for (uint8_t i = 0; i < StripLedPanel::k_pixel_count; ++i) {
        EXPECT_EQ(strip.get_pixel(i), RgbColor(0, 0, 0));
    }
}
