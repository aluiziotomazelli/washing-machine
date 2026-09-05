#include "discrete_led_panel.hpp"
#include "../hal/interfaces/i_reversible_motor.hpp"

namespace ui {

DiscreteLedPanel::DiscreteLedPanel(
    hal::IGpioHAL& gpio_hal,
    hal::ITimerHAL& timer_hal,
    const DiscreteLedPins& pins,
    bool active_high
)
    : gpio_hal_(gpio_hal)
    , timer_hal_(timer_hal)
    , pins_(pins)
    , active_high_(active_high)
    , is_initialized_(false)
    , current_program_(WashProgram::NORMAL_WASH)
    , is_blinking_wash_(false)
    , is_blinking_error_(false)
    , blink_state_(false)
    , last_blink_time_ms_(0)
{
}

void DiscreteLedPanel::init()
{
    if (pins_.power != 255)
        gpio_hal_.set_mode(pins_.power, hal::GpioMode::MODE_OUTPUT);
    if (pins_.softener != 255)
        gpio_hal_.set_mode(pins_.softener, hal::GpioMode::MODE_OUTPUT);
    if (pins_.wash != 255)
        gpio_hal_.set_mode(pins_.wash, hal::GpioMode::MODE_OUTPUT);
    if (pins_.rinse != 255)
        gpio_hal_.set_mode(pins_.rinse, hal::GpioMode::MODE_OUTPUT);
    if (pins_.spin != 255)
        gpio_hal_.set_mode(pins_.spin, hal::GpioMode::MODE_OUTPUT);
    if (pins_.level_low != 255)
        gpio_hal_.set_mode(pins_.level_low, hal::GpioMode::MODE_OUTPUT);
    if (pins_.level_med != 255)
        gpio_hal_.set_mode(pins_.level_med, hal::GpioMode::MODE_OUTPUT);
    if (pins_.level_high != 255)
        gpio_hal_.set_mode(pins_.level_high, hal::GpioMode::MODE_OUTPUT);

    turn_off_all();
    is_initialized_ = true;
}

void DiscreteLedPanel::update()
{
    if (!is_initialized_) {
        return;
    }

    if (!is_blinking_wash_ && !is_blinking_error_ && !is_blinking_power_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    if (now - last_blink_time_ms_ >= k_blink_interval_ms) {
        last_blink_time_ms_ = now;
        blink_state_ = !blink_state_;

        if (is_blinking_wash_) {
            write_pin(pins_.wash, blink_state_);
        }

        if (is_blinking_error_) {
            if (current_error_ == MachineError::DRAIN_TIMEOUT) {
                write_pin(pins_.wash, blink_state_);
                write_pin(pins_.rinse, blink_state_);
                write_pin(pins_.spin, blink_state_);
            } else if (current_error_ == MachineError::UNBALANCED_LOAD) {
                write_pin(pins_.spin, blink_state_);
            } else {
                write_pin(pins_.level_low, blink_state_);
                write_pin(pins_.level_med, blink_state_);
            }
        }

        if (is_blinking_power_) {
            write_pin(pins_.power, blink_state_);
        }
    }
}

void DiscreteLedPanel::set_softener(bool enabled)
{
    write_pin(pins_.softener, enabled);
}

void DiscreteLedPanel::set_program(WashProgram program)
{
    current_program_ = program;

    switch (program) {
    case WashProgram::NORMAL_WASH:
        is_blinking_wash_ = false;
        write_pin(pins_.wash, true);
        write_pin(pins_.rinse, false);
        write_pin(pins_.spin, false);
        break;

    case WashProgram::HEAVY_WASH:
        is_blinking_wash_ = true;
        blink_state_ = true;
        last_blink_time_ms_ = timer_hal_.get_time_ms();
        write_pin(pins_.wash, true);
        write_pin(pins_.rinse, false);
        write_pin(pins_.spin, false);
        break;

    case WashProgram::RINSE_ONLY:
        is_blinking_wash_ = false;
        write_pin(pins_.wash, false);
        write_pin(pins_.rinse, true);
        write_pin(pins_.spin, false);
        break;

    case WashProgram::SPIN_ONLY:
        is_blinking_wash_ = false;
        write_pin(pins_.wash, false);
        write_pin(pins_.rinse, false);
        write_pin(pins_.spin, true);
        break;
    }
}

void DiscreteLedPanel::set_stage(WashStage stage)
{
    is_blinking_wash_ = false; // Stop selection blinking when running stages

    switch (stage) {
    case WashStage::WASH:
        write_pin(pins_.wash, true);
        write_pin(pins_.rinse, false);
        write_pin(pins_.spin, false);
        break;
    case WashStage::RINSE:
        write_pin(pins_.wash, false);
        write_pin(pins_.rinse, true);
        write_pin(pins_.spin, false);
        break;
    case WashStage::SPIN:
        write_pin(pins_.wash, false);
        write_pin(pins_.rinse, false);
        write_pin(pins_.spin, true);
        break;
    case WashStage::IDLE:
    default:
        write_pin(pins_.wash, false);
        write_pin(pins_.rinse, false);
        write_pin(pins_.spin, false);
        break;
    }
}

void DiscreteLedPanel::set_selected_level(WaterLevel level)
{
    switch (level) {
    case WaterLevel::LOW_LEVEL:
        write_pin(pins_.level_low, true);
        write_pin(pins_.level_med, false);
        write_pin(pins_.level_high, false);
        break;
    case WaterLevel::MEDIUM_LEVEL:
        write_pin(pins_.level_low, false);
        write_pin(pins_.level_med, true);
        write_pin(pins_.level_high, false);
        break;
    case WaterLevel::HIGH_LEVEL:
        if (pins_.level_high != 255) {
            write_pin(pins_.level_low, false);
            write_pin(pins_.level_med, false);
            write_pin(pins_.level_high, true);
        } else {
            write_pin(pins_.level_low, true);
            write_pin(pins_.level_med, true);
        }
        break;
    case WaterLevel::EMPTY:
    default:
        write_pin(pins_.level_low, false);
        write_pin(pins_.level_med, false);
        write_pin(pins_.level_high, false);
        break;
    }
}

void DiscreteLedPanel::set_machine_state(MachineState state, MachineError error)
{
    current_error_ = error;

    switch (state) {
    case MachineState::IDLE:
    case MachineState::FINISHED:
        is_blinking_power_ = false;
        is_blinking_error_ = false;
        write_pin(pins_.power, false);
        break;

    case MachineState::RUNNING:
        is_blinking_power_ = false;
        is_blinking_error_ = false;
        write_pin(pins_.power, true);
        break;

    case MachineState::PAUSED:
        is_blinking_power_ = true;
        is_blinking_error_ = false;
        blink_state_ = true;
        last_blink_time_ms_ = timer_hal_.get_time_ms();
        write_pin(pins_.power, true);
        break;

    case MachineState::ERROR:
        is_blinking_power_ = false;
        is_blinking_error_ = true;
        blink_state_ = true;
        last_blink_time_ms_ = timer_hal_.get_time_ms();
        write_pin(pins_.power, false);

        if (current_error_ == MachineError::FILL_TIMEOUT) {
            // Fill error (legacy type 1): blink all water level LEDs, turn off stage LEDs
            write_pin(pins_.wash, false);
            write_pin(pins_.rinse, false);
            write_pin(pins_.spin, false);
            write_pin(pins_.level_low, true);
            write_pin(pins_.level_med, true);
        } else if (current_error_ == MachineError::DRAIN_TIMEOUT) {
            // Drain error (legacy type 2): blink all program/stage LEDs, turn off level LEDs
            write_pin(pins_.level_low, false);
            write_pin(pins_.level_med, false);
            write_pin(pins_.wash, true);
            write_pin(pins_.rinse, true);
            write_pin(pins_.spin, true);
        } else if (current_error_ == MachineError::UNBALANCED_LOAD) {
            // Unbalance error: blink spin LED only, turn off others
            write_pin(pins_.level_low, false);
            write_pin(pins_.level_med, false);
            write_pin(pins_.wash, false);
            write_pin(pins_.rinse, false);
            write_pin(pins_.spin, true);
        } else {
            // Generic error: blink level LEDs
            write_pin(pins_.level_low, true);
            write_pin(pins_.level_med, true);
        }
        break;
    }
}

void DiscreteLedPanel::turn_off_all()
{
    is_blinking_wash_ = false;
    is_blinking_error_ = false;
    is_blinking_power_ = false;
    current_error_ = MachineError::NONE;

    write_pin(pins_.power, false);
    write_pin(pins_.softener, false);
    write_pin(pins_.wash, false);
    write_pin(pins_.rinse, false);
    write_pin(pins_.spin, false);
    write_pin(pins_.level_low, false);
    write_pin(pins_.level_med, false);
    write_pin(pins_.level_high, false);
}

void DiscreteLedPanel::write_pin(uint8_t pin, bool state)
{
    if (pin == 255) {
        return;
    }
    bool level = active_high_ ? state : !state;
    gpio_hal_.set_level(pin, level ? hal::GpioLevel::LEVEL_HIGH : hal::GpioLevel::LEVEL_LOW);
}

void DiscreteLedPanel::show_diagnostic(DiagnosticStep step, uint16_t raw_value, bool status_ok)
{
    turn_off_all();

    switch (step) {
    case DiagnosticStep::LEVEL_SENSOR:
        set_selected_level(static_cast<WaterLevel>(raw_value));
        write_pin(pins_.spin, true); // Status indicator for step 1
        break;

    case DiagnosticStep::VIBRATION_SENSOR:
        write_pin(pins_.rinse, true); // Status indicator for step 2
        write_pin(pins_.softener, status_ok);

        if (status_ok) {
            if (raw_value >= 400) {
                write_pin(pins_.level_low, true);
            }
            if (raw_value >= 5000) {
                write_pin(pins_.level_med, true);
            }
            if (raw_value >= 9000) {
                if (pins_.level_high != 255) {
                    write_pin(pins_.level_high, true);
                }
            }
        }
        break;

    case DiagnosticStep::MAIN_VALVE:
        write_pin(pins_.wash, true); // Status indicator for step 3
        if (raw_value != 0) {
            write_pin(pins_.level_low, true);
            write_pin(pins_.level_med, true);
        }
        break;

    case DiagnosticStep::SOFTENER_VALVE:
        write_pin(pins_.wash, true); // Status indicator for step 4
        if (raw_value != 0) {
            write_pin(pins_.softener, true);
        }
        break;

    case DiagnosticStep::DRAIN_PUMP:
        set_selected_level(static_cast<WaterLevel>(raw_value));
        write_pin(pins_.spin, true);          // Step indicator
        write_pin(pins_.softener, status_ok); // Pump active indicator
        break;

    case DiagnosticStep::MOTOR_AGITATE:
        write_pin(pins_.wash, true); // Step indicator & CCW indicator
        if (raw_value == static_cast<uint16_t>(hal::MotorState::RUNNING_CLOCKWISE)) {
            write_pin(pins_.spin, true); // CW indicator
        }
        break;

    case DiagnosticStep::SPIN_TEST:
        write_pin(pins_.spin, true); // Step indicator
        if (!status_ok) {
            write_pin(pins_.softener, true); // Vibration trip warning indicator
        } else {
            write_pin(pins_.rinse, (raw_value & 0x01) != 0); // Pump active indicator
            write_pin(pins_.wash, (raw_value & 0x02) != 0);  // Motor active indicator
        }
        break;

    default:
        break;
    }
}

} // namespace ui
