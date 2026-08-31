#pragma once

#include <stdint.h>

namespace ui {

/**
 * @brief Types of button click events detected by the FSM debouncer.
 */
enum class ButtonClickType : uint8_t {
    NONE_CLICK = 0,
    CLICK,
    DOUBLE_CLICK,
    LONG_CLICK,
    VERY_LONG_CLICK,
    TIMEOUT,
    ERROR_STATE
};

/**
 * @brief Configuration parameters for electrical behavior and timing thresholds.
 */
struct ButtonConfig {
    bool active_low{true};
    bool enable_internal_pull{true};
    uint16_t debounce_press_ms{20};
    uint16_t debounce_release_ms{20};
    uint16_t double_click_ms{300};
    uint16_t long_click_ms{1000};
    uint16_t very_long_click_ms{3000};
    uint16_t timeout_ms{6000};
};

/**
 * @interface IButton
 * @brief Abstract interface for button inputs.
 */
class IButton {
public:
    virtual ~IButton() = default;

    /**
     * @brief Initialize hardware pin mode and internal state.
     */
    virtual void init() = 0;

    /**
     * @brief Deinitialize pin if needed.
     */
    virtual void deinit() = 0;

    /**
     * @brief Periodic non-blocking state machine update (to be called in loop).
     */
    virtual void update() = 0;

    /**
     * @brief Consume and return the last detected click event.
     * @return ButtonClickType The event detected, resets to NONE_CLICK on read.
     */
    virtual ButtonClickType get_last_click() = 0;

    /**
     * @brief Check whether the button is currently physically depressed.
     * @return true if currently pressed, false otherwise.
     */
    virtual bool is_pressed() const = 0;
};

} // namespace ui
