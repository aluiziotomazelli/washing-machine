#pragma once

#include "interfaces/i_led_panel.hpp"
#include "../hal/interfaces/i_rgb_strip.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace ui {

/**
 * @brief Configuration parameters for WS2812 strip visual presentation.
 */
struct StripLedConfig
{
    uint8_t max_brightness{80};       //!< Global brightness cap (0..255; 80 is roughly 30%).
    uint8_t idle_brightness{60};      //!< Steady brightness during IDLE selection.
    uint8_t future_brightness{15};    //!< Dim brightness for upcoming stages during RUNNING.
    uint16_t breathe_period_ms{2000}; //!< Breathing animation period during RUNNING.
    uint16_t blink_interval_ms{500};  //!< Strobe period during PAUSED and ERROR.
    uint8_t frame_interval_ms{33};    //!< Frame throttle interval (~30 FPS).

    constexpr StripLedConfig() = default;

    constexpr StripLedConfig(
        uint8_t max_b,
        uint8_t idle_b = 60,
        uint8_t future_b = 15,
        uint16_t breathe_ms = 2000,
        uint16_t blink_ms = 500,
        uint8_t frame_ms = 33)
        : max_brightness(max_b)
        , idle_brightness(idle_b)
        , future_brightness(future_b)
        , breathe_period_ms(breathe_ms)
        , blink_interval_ms(blink_ms)
        , frame_interval_ms(frame_ms)
    {
    }
};

/**
 * @class StripLedPanel
 * @brief 9-Pixel Addressable RGB LED Panel Driver implementing ui::ILedPanel.
 *
 * Pixel Mapping:
 * [P0]: Spin Stage (Warm white)
 * [P1]: Rinse Stage (Warm white)
 * [P2]: Wash Stage (Warm white)
 * [P3]: Heavy Wash / Soak Indicator (Warm white)
 * [P4]: Gap / Separator (OFF)
 * [P5]: High Level (Cyan)
 * [P6]: Medium Level (Cyan)
 * [P7]: Low Level (Cyan)
 * [P8]: Softener (Pink)
 */
class StripLedPanel : public ILedPanel
{
public:
    static constexpr uint8_t k_pixel_count = 9;

    // Pixel indices (Inverted strip orientation: P0 at the right, P8 at the left)
    static constexpr uint8_t k_idx_spin = 0;
    static constexpr uint8_t k_idx_rinse = 1;
    static constexpr uint8_t k_idx_wash = 2;
    static constexpr uint8_t k_idx_heavy_wash = 3;
    static constexpr uint8_t k_idx_gap1 = 4;
    static constexpr uint8_t k_idx_lvl_high = 5;
    static constexpr uint8_t k_idx_lvl_med = 6;
    static constexpr uint8_t k_idx_lvl_low = 7;
    static constexpr uint8_t k_idx_softener = 8;

    // Color definitions
    static constexpr hal::RgbColor k_color_pink{255, 30, 140}; // Soft pastel rose (less saturated)
    static constexpr hal::RgbColor k_color_cyan{0, 220, 255};
    static constexpr hal::RgbColor k_color_white{255, 197, 120};
    static constexpr hal::RgbColor k_color_red{255, 0, 0};
    static constexpr hal::RgbColor k_color_blue{0, 100, 255};
    static constexpr hal::RgbColor k_color_green{0, 255, 0};
    static constexpr hal::RgbColor k_color_yellow{255, 180, 0};
    static constexpr hal::RgbColor k_color_magenta{255, 0, 200};
    static constexpr hal::RgbColor k_color_off{0, 0, 0};

    /**
     * @brief Construct a strip LED panel.
     * @param strip Addressable RGB strip driver.
     * @param timer_hal Timer abstraction used for animation timing.
     * @param config Visual animation configuration.
     */
    StripLedPanel(hal::IRgbStrip& strip, hal::ITimerHAL& timer_hal, const StripLedConfig& config = StripLedConfig{});

    ~StripLedPanel() override = default;

    /** @copydoc ILedPanel::init */
    void init() override;
    /** @copydoc ILedPanel::update */
    void update() override;

    /** @copydoc ILedPanel::set_machine_state */
    void
    set_machine_state(domain::MachineState state, domain::MachineError error = domain::MachineError::NONE) override;
    /** @copydoc ILedPanel::set_softener */
    void set_softener(bool enabled) override;
    /** @copydoc ILedPanel::set_program */
    void set_program(WashProgram program) override;
    /** @copydoc ILedPanel::set_stage */
    void set_stage(WashStage stage) override;
    /** @copydoc ILedPanel::set_selected_level */
    void set_selected_level(WaterLevel level) override;
    /** @copydoc ILedPanel::turn_off_all */
    void turn_off_all() override;
    /** @copydoc ILedPanel::show_diagnostic */
    void show_diagnostic(DiagnosticStep step, uint16_t raw_value = 0, bool status_ok = true) override;

    /**
     * @brief Get the immutable visual configuration in use.
     * @return Reference to the active StripLedConfig.
     */
    const StripLedConfig& get_config() const { return config_; }

private:
    void render_frame(uint32_t now);
    void apply_color(uint8_t index, hal::RgbColor base_color, uint8_t brightness);
    uint8_t calculate_breathe_brightness(uint32_t now) const;

    hal::IRgbStrip& strip_;
    hal::ITimerHAL& timer_hal_;
    StripLedConfig config_;

    domain::MachineState state_{domain::MachineState::IDLE};
    domain::MachineError error_{domain::MachineError::NONE};
    domain::WashProgram program_{domain::WashProgram::RINSE_ONLY};
    domain::WashStage stage_{domain::WashStage::IDLE};
    domain::WaterLevel level_{domain::WaterLevel::LOW_LEVEL};
    bool softener_enabled_{false};

    bool is_initialized_{false};
    bool blink_state_{true};
    uint32_t last_blink_time_ms_{0};
    uint32_t last_frame_time_ms_{0};
};

} // namespace ui
