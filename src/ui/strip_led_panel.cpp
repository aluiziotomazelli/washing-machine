#include "strip_led_panel.hpp"

namespace ui {

constexpr hal::RgbColor StripLedPanel::k_color_pink;
constexpr hal::RgbColor StripLedPanel::k_color_cyan;
constexpr hal::RgbColor StripLedPanel::k_color_white;
constexpr hal::RgbColor StripLedPanel::k_color_red;
constexpr hal::RgbColor StripLedPanel::k_color_off;

StripLedPanel::StripLedPanel(
    hal::IRgbStrip& strip,
    hal::ITimerHAL& timer_hal,
    const StripLedConfig& config
)
    : strip_(strip),
      timer_hal_(timer_hal),
      config_(config)
{
}

void StripLedPanel::init()
{
    strip_.init();
    is_initialized_ = true;
    last_blink_time_ms_ = timer_hal_.get_time_ms();
    blink_state_ = true;
    render_frame(last_blink_time_ms_);
}

void StripLedPanel::set_machine_state(domain::MachineState state, domain::MachineError error)
{
    state_ = state;
    error_ = error;
    last_blink_time_ms_ = timer_hal_.get_time_ms();
    blink_state_ = true;
    render_frame(last_blink_time_ms_);
}

void StripLedPanel::set_softener(bool enabled)
{
    softener_enabled_ = enabled;
    render_frame(timer_hal_.get_time_ms());
}

void StripLedPanel::set_program(WashProgram program)
{
    program_ = program;
    render_frame(timer_hal_.get_time_ms());
}

void StripLedPanel::set_stage(WashStage stage)
{
    stage_ = stage;
    render_frame(timer_hal_.get_time_ms());
}

void StripLedPanel::set_selected_level(WaterLevel level)
{
    level_ = level;
    render_frame(timer_hal_.get_time_ms());
}

void StripLedPanel::turn_off_all()
{
    state_ = domain::MachineState::IDLE;
    error_ = domain::MachineError::NONE;
    strip_.clear();
    strip_.show();
}

void StripLedPanel::update()
{
    if (!is_initialized_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();

    // Rate limiter: render frame only at configured interval (e.g. 20ms -> 50 FPS)
    if (now - last_frame_time_ms_ < config_.frame_interval_ms) {
        return;
    }
    last_frame_time_ms_ = now;

    if (now - last_blink_time_ms_ >= config_.blink_interval_ms) {
        last_blink_time_ms_ = now;
        blink_state_ = !blink_state_;
    }

    render_frame(now);
}

void StripLedPanel::apply_color(uint8_t index, hal::RgbColor base_color, uint8_t brightness)
{
    // Global max_brightness scaling: (brightness * max_brightness) / 255
    uint8_t scaled_b = static_cast<uint8_t>((static_cast<uint16_t>(brightness) * config_.max_brightness) / 255);

    uint8_t r = static_cast<uint8_t>((static_cast<uint16_t>(base_color.r) * scaled_b) / 255);
    uint8_t g = static_cast<uint8_t>((static_cast<uint16_t>(base_color.g) * scaled_b) / 255);
    uint8_t b = static_cast<uint8_t>((static_cast<uint16_t>(base_color.b) * scaled_b) / 255);

    strip_.set_pixel(index, r, g, b);
}

uint8_t StripLedPanel::calculate_breathe_brightness(uint32_t now) const
{
    uint32_t period = config_.breathe_period_ms > 0 ? config_.breathe_period_ms : 2000;
    uint32_t phase = (now % period) * 512 / period;
    uint8_t wave = (phase < 256) ? static_cast<uint8_t>(phase) : static_cast<uint8_t>(511 - phase);

    // Scale between 20% (51/255) and 100% (255/255)
    return static_cast<uint8_t>(51 + ((255 - 51) * wave) / 255);
}

void StripLedPanel::render_frame(uint32_t now)
{
    strip_.clear();

    if (state_ == domain::MachineState::FINISHED) {
        strip_.show();
        return;
    }

    // -------------------------------------------------------------------------
    // 1. Softener (P0 - Pink)
    // -------------------------------------------------------------------------
    if (softener_enabled_ && state_ != domain::MachineState::ERROR) {
        if (state_ != domain::MachineState::PAUSED || blink_state_) {
            apply_color(k_idx_softener, k_color_pink, config_.idle_brightness);
        }
    }

    // -------------------------------------------------------------------------
    // 2. Water Level (P2, P3, P4 - Cyan or Red on Error)
    // -------------------------------------------------------------------------
    if (state_ == domain::MachineState::ERROR) {
        if (error_ == domain::MachineError::FILL_TIMEOUT) {
            // Water inlet error: blink level LEDs in RED
            if (blink_state_) {
                apply_color(k_idx_lvl_low, k_color_red, 255);
                apply_color(k_idx_lvl_med, k_color_red, 255);
                apply_color(k_idx_lvl_high, k_color_red, 255);
            }
        }
        // When DRAIN_TIMEOUT, level LEDs stay completely OFF
    }
    else {
        // Normal level display
        if (state_ != domain::MachineState::PAUSED || blink_state_) {
            switch (level_) {
            case domain::WaterLevel::LOW_LEVEL:
                apply_color(k_idx_lvl_low, k_color_cyan, config_.idle_brightness);
                break;
            case domain::WaterLevel::MEDIUM_LEVEL:
                apply_color(k_idx_lvl_low, k_color_cyan, config_.idle_brightness);
                apply_color(k_idx_lvl_med, k_color_cyan, config_.idle_brightness);
                break;
            case domain::WaterLevel::HIGH_LEVEL:
                apply_color(k_idx_lvl_low, k_color_cyan, config_.idle_brightness);
                apply_color(k_idx_lvl_med, k_color_cyan, config_.idle_brightness);
                apply_color(k_idx_lvl_high, k_color_cyan, config_.idle_brightness);
                break;
            case domain::WaterLevel::EMPTY:
                break;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 3. Programs & Stages (P5, P6, P7, P8 - White or Red on Error)
    // -------------------------------------------------------------------------
    if (state_ == domain::MachineState::ERROR) {
        if (error_ == domain::MachineError::DRAIN_TIMEOUT) {
            // Drain error: blink all program LEDs in RED
            if (blink_state_) {
                apply_color(k_idx_wash, k_color_red, 255);
                apply_color(k_idx_rinse, k_color_red, 255);
                apply_color(k_idx_spin, k_color_red, 255);
            }
        }
        // When FILL_TIMEOUT, program LEDs stay completely OFF
    }
    else if (state_ == domain::MachineState::IDLE) {
        // Selection mode: steady display of selected program in White
        switch (program_) {
        case domain::WashProgram::NORMAL_WASH:
            apply_color(k_idx_wash, k_color_white, config_.idle_brightness);
            apply_color(k_idx_rinse, k_color_white, config_.idle_brightness);
            apply_color(k_idx_spin, k_color_white, config_.idle_brightness);
            break;
        case domain::WashProgram::HEAVY_WASH:
            apply_color(k_idx_heavy_wash, k_color_white, config_.idle_brightness);
            apply_color(k_idx_wash, k_color_white, config_.idle_brightness);
            apply_color(k_idx_rinse, k_color_white, config_.idle_brightness);
            apply_color(k_idx_spin, k_color_white, config_.idle_brightness);
            break;
        case domain::WashProgram::RINSE_ONLY:
            apply_color(k_idx_rinse, k_color_white, config_.idle_brightness);
            apply_color(k_idx_spin, k_color_white, config_.idle_brightness);
            break;
        case domain::WashProgram::SPIN_ONLY:
            apply_color(k_idx_spin, k_color_white, config_.idle_brightness);
            break;
        }
    }
    else if (state_ == domain::MachineState::RUNNING) {
        // Running mode: Active stage breathes smoothly, future stages glow dim
        uint8_t breathe_b = calculate_breathe_brightness(now);
        uint8_t dim_b = config_.future_brightness;

        switch (stage_) {
        case domain::WashStage::WASH:
            if (program_ == domain::WashProgram::HEAVY_WASH) {
                apply_color(k_idx_heavy_wash, k_color_white, breathe_b);
            }
            apply_color(k_idx_wash, k_color_white, breathe_b);
            apply_color(k_idx_rinse, k_color_white, dim_b);
            apply_color(k_idx_spin, k_color_white, dim_b);
            break;

        case domain::WashStage::RINSE:
            apply_color(k_idx_rinse, k_color_white, breathe_b);
            apply_color(k_idx_spin, k_color_white, dim_b);
            break;

        case domain::WashStage::SPIN:
            apply_color(k_idx_spin, k_color_white, breathe_b);
            break;

        case domain::WashStage::IDLE:
            break;
        }
    }
    else if (state_ == domain::MachineState::PAUSED) {
        // Paused mode: current running stage blinks synchronously
        if (blink_state_) {
            switch (stage_) {
            case domain::WashStage::WASH:
                if (program_ == domain::WashProgram::HEAVY_WASH) {
                    apply_color(k_idx_heavy_wash, k_color_white, config_.idle_brightness);
                }
                apply_color(k_idx_wash, k_color_white, config_.idle_brightness);
                apply_color(k_idx_rinse, k_color_white, config_.future_brightness);
                apply_color(k_idx_spin, k_color_white, config_.future_brightness);
                break;

            case domain::WashStage::RINSE:
                apply_color(k_idx_rinse, k_color_white, config_.idle_brightness);
                apply_color(k_idx_spin, k_color_white, config_.future_brightness);
                break;

            case domain::WashStage::SPIN:
                apply_color(k_idx_spin, k_color_white, config_.idle_brightness);
                break;

            case domain::WashStage::IDLE:
                break;
            }
        }
    }

    strip_.show();
}

} // namespace ui
