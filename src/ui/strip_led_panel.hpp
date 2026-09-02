#pragma once

#include "interfaces/i_led_panel.hpp"
#include "../hal/interfaces/i_rgb_strip.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace ui {

/**
 * @brief Configuration parameters for WS2812 strip visual presentation.
 */
struct StripLedConfig {
    uint8_t max_brightness{80};        // Global max brightness cap (0..255) -> 80 is ~30%
    uint8_t idle_brightness{60};       // Steady brightness during IDLE selection
    uint8_t future_brightness{15};     // Dim brightness for upcoming stages during RUNNING
    uint16_t breathe_period_ms{2000};  // Period of smooth breathing animation in RUNNING (2s)
    uint16_t blink_interval_ms{500};   // Period of strobe in PAUSED and ERROR (500ms)
    uint8_t frame_interval_ms{33};     // Frame throttle interval (33ms -> ~30 FPS)
};

/**
 * @class StripLedPanel
 * @brief 9-Pixel Addressable RGB LED Panel Driver implementing ui::ILedPanel.
 * 
 * Pixel Mapping:
 * [P0]: Softener (Pink)
 * [P1]: Gap / Separator (OFF)
 * [P2]: Low Level (Cyan)
 * [P3]: Medium Level (Cyan)
 * [P4]: High Level (Cyan)
 * [P5]: Heavy Wash / Soak Indicator (White - selection & soak)
 * [P6]: Wash Stage (White - L)
 * [P7]: Rinse Stage (White - E)
 * [P8]: Spin Stage (White - C)
 */
class StripLedPanel : public ILedPanel {
public:
    static constexpr uint8_t k_pixel_count = 9;

    // Pixel indices (Inverted strip orientation: P0 at the right, P8 at the left)
    static constexpr uint8_t k_idx_spin       = 0;
    static constexpr uint8_t k_idx_rinse      = 1;
    static constexpr uint8_t k_idx_wash       = 2;
    static constexpr uint8_t k_idx_heavy_wash = 3;
    static constexpr uint8_t k_idx_lvl_high   = 4;
    static constexpr uint8_t k_idx_lvl_med    = 5;
    static constexpr uint8_t k_idx_lvl_low    = 6;
    static constexpr uint8_t k_idx_gap1       = 7;
    static constexpr uint8_t k_idx_softener   = 8;

    // Color definitions
    static constexpr hal::RgbColor k_color_pink{255, 20, 140};
    static constexpr hal::RgbColor k_color_cyan{0, 220, 255};
    static constexpr hal::RgbColor k_color_white{255, 255, 255};
    static constexpr hal::RgbColor k_color_red{255, 0, 0};
    static constexpr hal::RgbColor k_color_off{0, 0, 0};

    StripLedPanel(
        hal::IRgbStrip& strip,
        hal::ITimerHAL& timer_hal,
        const StripLedConfig& config = StripLedConfig{}
    );

    ~StripLedPanel() override = default;

    void init() override;
    void update() override;

    void set_machine_state(domain::MachineState state, domain::MachineError error = domain::MachineError::NONE) override;
    void set_softener(bool enabled) override;
    void set_program(WashProgram program) override;
    void set_stage(WashStage stage) override;
    void set_selected_level(WaterLevel level) override;
    void turn_off_all() override;

    // Getter for configuration
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
