#include "discrete_led_panel.hpp"

namespace ui {

DiscreteLedPanel::DiscreteLedPanel(hal::IGpioHAL& gpio_hal, const DiscreteLedPins& pins, bool active_high)
    : gpio_hal_(gpio_hal)
    , pins_(pins)
    , active_high_(active_high)
    , is_initialized_(false)
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
    // No dynamic animation needed for discrete static LEDs
}

void DiscreteLedPanel::set_power(bool on)
{
    write_pin(pins_.power, on);
}

void DiscreteLedPanel::set_softener(bool enabled)
{
    write_pin(pins_.softener, enabled);
}

void DiscreteLedPanel::set_stage(WashStage stage)
{
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

void DiscreteLedPanel::set_selected_level(hal::WaterLevel level)
{
    switch (level) {
    case hal::WaterLevel::LOW_LEVEL:
        write_pin(pins_.level_low, true);
        write_pin(pins_.level_med, false);
        write_pin(pins_.level_high, false);
        break;
    case hal::WaterLevel::MEDIUM_LEVEL:
        write_pin(pins_.level_low, false);
        write_pin(pins_.level_med, true);
        write_pin(pins_.level_high, false);
        break;
    case hal::WaterLevel::HIGH_LEVEL:
        if (pins_.level_high != 255) {
            write_pin(pins_.level_low, false);
            write_pin(pins_.level_med, false);
            write_pin(pins_.level_high, true);
        }
        else {
            // If only 2 physical level LEDs are mounted, light both for High level
            write_pin(pins_.level_low, true);
            write_pin(pins_.level_med, true);
        }
        break;
    case hal::WaterLevel::EMPTY:
    default:
        write_pin(pins_.level_low, false);
        write_pin(pins_.level_med, false);
        write_pin(pins_.level_high, false);
        break;
    }
}

void DiscreteLedPanel::set_error(bool error)
{
    if (error) {
        set_power(true);
    }
}

void DiscreteLedPanel::turn_off_all()
{
    write_pin(pins_.power, false);
    write_pin(pins_.softener, false);
    write_pin(pins_.wash, false);
    write_pin(pins_.rinse, false);
    write_pin(pins_.spin, false);
    write_pin(pins_.level_low, false);
    write_pin(pins_.level_med, false);
    write_pin(pins_.level_high, false);
}

void DiscreteLedPanel::write_pin(uint8_t pin, bool on)
{
    if (pin == 255) {
        return;
    }
    bool physical_high = active_high_ ? on : !on;
    gpio_hal_.set_level(pin, physical_high ? hal::GpioLevel::LEVEL_HIGH : hal::GpioLevel::LEVEL_LOW);
}

} // namespace ui
