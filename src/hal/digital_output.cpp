#include "digital_output.hpp"

namespace hal {

DigitalOutput* DigitalOutput::head_ = nullptr;

DigitalOutput::DigitalOutput(
    IGpioHAL& gpio_hal,
    uint8_t pin,
    bool active_low,
    bool initial_state
)
    : gpio_hal_(gpio_hal)
    , pin_(pin)
    , active_low_(active_low)
    , is_on_(initial_state)
    , is_initialized_(false)
{
    // Auto-registration in intrusive static linked list
    next_ = head_;
    head_ = this;
}

void DigitalOutput::init()
{
    gpio_hal_.set_mode(pin_, GpioMode::MODE_OUTPUT);
    apply_hardware_level();
    is_initialized_ = true;
}

void DigitalOutput::turn_on()
{
    if (!is_on_ || !is_initialized_) {
        is_on_ = true;
        apply_hardware_level();
    }
}

void DigitalOutput::turn_off()
{
    if (is_on_ || !is_initialized_) {
        is_on_ = false;
        apply_hardware_level();
    }
}

void DigitalOutput::toggle()
{
    is_on_ = !is_on_;
    apply_hardware_level();
}

bool DigitalOutput::is_on() const
{
    return is_on_;
}

uint8_t DigitalOutput::get_pin() const
{
    return pin_;
}

void DigitalOutput::apply_hardware_level()
{
    // If active_low: ON -> LEVEL_LOW, OFF -> LEVEL_HIGH
    // If active_high: ON -> LEVEL_HIGH, OFF -> LEVEL_LOW
    bool physical_high = active_low_ ? !is_on_ : is_on_;
    gpio_hal_.set_level(pin_, physical_high ? GpioLevel::LEVEL_HIGH : GpioLevel::LEVEL_LOW);
}

void DigitalOutput::init_all()
{
    for (DigitalOutput* curr = head_; curr != nullptr; curr = curr->next_) {
        curr->init();
    }
}

void DigitalOutput::turn_off_all()
{
    for (DigitalOutput* curr = head_; curr != nullptr; curr = curr->next_) {
        curr->turn_off();
    }
}

void DigitalOutput::reset_registry()
{
    head_ = nullptr;
}

} // namespace hal
