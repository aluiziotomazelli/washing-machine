#pragma once

#include <stdint.h>
#include "interfaces/i_button.hpp"
#include "../hal/interfaces/i_gpio_hal.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace ui {

/**
 * @class Button
 * @brief Platform-agnostic button driver with non-blocking FSM debouncing, multi-click and long-press support.
 */
class Button : public IButton {
public:
    Button(
        hal::IGpioHAL& gpio_hal,
        hal::ITimerHAL& timer_hal,
        uint8_t pin,
        const ButtonConfig& config = ButtonConfig{});

    ~Button() override = default;

    void init() override;
    void deinit() override;
    void update() override;
    ButtonClickType get_last_click() override;
    bool is_pressed() const override;

private:
    enum class State : uint8_t {
        WAIT_FOR_PRESS,
        DEBOUNCE_PRESS,
        WAIT_FOR_RELEASE,
        DEBOUNCE_RELEASE,
        WAIT_FOR_DOUBLE,
        TIMEOUT_WAIT_FOR_RELEASE
    };

    hal::IGpioHAL& gpio_hal_;
    hal::ITimerHAL& timer_hal_;
    uint8_t pin_;
    ButtonConfig config_;

    State state_{State::WAIT_FOR_PRESS};
    uint32_t last_time_ms_{0};
    uint32_t press_start_time_ms_{0};
    bool first_click_{false};
    bool is_initialized_{false};

    ButtonClickType last_click_type_{ButtonClickType::NONE_CLICK};
};

} // namespace ui
