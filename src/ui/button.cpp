#include "button.hpp"

namespace ui {

Button::Button(
    hal::IGpioHAL& gpio_hal,
    hal::ITimerHAL& timer_hal,
    uint8_t pin,
    const ButtonConfig& config)
    : gpio_hal_(gpio_hal)
    , timer_hal_(timer_hal)
    , pin_(pin)
    , config_(config)
    , state_(State::WAIT_FOR_PRESS)
    , last_time_ms_(0)
    , press_start_time_ms_(0)
    , first_click_(false)
    , is_initialized_(false)
    , last_click_type_(ButtonClickType::NONE_CLICK)
{
}

void Button::init()
{
    if (is_initialized_) {
        return;
    }

    if (config_.enable_internal_pull) {
        gpio_hal_.set_mode(pin_, config_.active_low ? hal::GpioMode::MODE_INPUT_PULLUP : hal::GpioMode::MODE_INPUT);
    } else {
        gpio_hal_.set_mode(pin_, hal::GpioMode::MODE_INPUT);
    }

    is_initialized_ = true;
}

void Button::deinit()
{
    if (!is_initialized_) {
        return;
    }

    gpio_hal_.set_mode(pin_, hal::GpioMode::MODE_INPUT);
    is_initialized_ = false;
}

bool Button::is_pressed() const
{
    if (!is_initialized_) {
        return false;
    }

    hal::GpioLevel current_level = gpio_hal_.get_level(pin_);
    hal::GpioLevel pressed_level = config_.active_low ? hal::GpioLevel::LEVEL_LOW : hal::GpioLevel::LEVEL_HIGH;
    return current_level == pressed_level;
}

void Button::update()
{
    if (!is_initialized_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    hal::GpioLevel pressed_level = config_.active_low ? hal::GpioLevel::LEVEL_LOW : hal::GpioLevel::LEVEL_HIGH;
    hal::GpioLevel released_level = config_.active_low ? hal::GpioLevel::LEVEL_HIGH : hal::GpioLevel::LEVEL_LOW;
    hal::GpioLevel current_level = gpio_hal_.get_level(pin_);

    switch (state_) {
    case State::WAIT_FOR_PRESS:
        if (current_level == pressed_level) {
            press_start_time_ms_ = now;
            state_ = State::DEBOUNCE_PRESS;
        }
        break;

    case State::DEBOUNCE_PRESS:
        if (now - press_start_time_ms_ >= config_.debounce_press_ms) {
            if (current_level == pressed_level) {
                state_ = State::WAIT_FOR_RELEASE;
            } else {
                state_ = State::WAIT_FOR_PRESS;
            }
        }
        break;

    case State::WAIT_FOR_RELEASE:
        if (current_level == released_level) {
            uint32_t duration = now - press_start_time_ms_;

            if (duration >= config_.very_long_click_ms) {
                state_ = State::WAIT_FOR_PRESS;
                last_click_type_ = ButtonClickType::VERY_LONG_CLICK;
            } else if (duration >= config_.long_click_ms) {
                state_ = State::WAIT_FOR_PRESS;
                last_click_type_ = ButtonClickType::LONG_CLICK;
            } else {
                last_time_ms_ = now;
                state_ = State::DEBOUNCE_RELEASE;
            }
        } else if (now - press_start_time_ms_ >= config_.timeout_ms) {
            state_ = State::TIMEOUT_WAIT_FOR_RELEASE;
            last_time_ms_ = now;
        }
        break;

    case State::DEBOUNCE_RELEASE:
        if (now - last_time_ms_ >= config_.debounce_release_ms) {
            state_ = State::WAIT_FOR_DOUBLE;
        }
        break;

    case State::WAIT_FOR_DOUBLE:
        if (current_level == pressed_level && !first_click_) {
            last_time_ms_ = now;
            first_click_ = true;
            state_ = State::DEBOUNCE_PRESS;
        } else if (now - last_time_ms_ >= config_.double_click_ms) {
            state_ = State::WAIT_FOR_PRESS;
            if (first_click_) {
                first_click_ = false;
                last_click_type_ = ButtonClickType::DOUBLE_CLICK;
            } else {
                last_click_type_ = ButtonClickType::CLICK;
            }
        }
        break;

    case State::TIMEOUT_WAIT_FOR_RELEASE:
        if (current_level == released_level) {
            if (now - last_time_ms_ >= config_.debounce_release_ms) {
                last_time_ms_ = now;
                state_ = State::WAIT_FOR_PRESS;
                last_click_type_ = ButtonClickType::TIMEOUT;
            }
        } else {
            last_time_ms_ = now;
            if (now - press_start_time_ms_ >= 2 * config_.timeout_ms) {
                state_ = State::WAIT_FOR_PRESS;
                last_click_type_ = ButtonClickType::ERROR_STATE;
            }
        }
        break;
    }
}

ButtonClickType Button::get_last_click()
{
    ButtonClickType click = last_click_type_;
    last_click_type_ = ButtonClickType::NONE_CLICK;
    return click;
}

} // namespace ui
