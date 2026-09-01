#pragma once

#include <stdint.h>

namespace ui {

/**
 * @brief Predefined audio feedback patterns for system feedback and alerts.
 */
enum class BuzzerPattern : uint8_t {
    NONE = 0,
    SHORT_BEEP,       // 50ms single beep (button press feedback)
    DOUBLE_BEEP,      // Two rapid beeps (mode change / special option)
    LONG_BEEP,        // 500ms continuous beep (stage skip / long press)
    CYCLE_FINISHED,   // 4 melodic beeps indicating wash cycle completion
    ERROR_ALARM       // Continuous repeating alarm indicating fault/timeout
};

/**
 * @interface IBuzzer
 * @brief Abstract interface for non-blocking audible alerts and feedback.
 */
class IBuzzer {
public:
    virtual ~IBuzzer() = default;

    /**
     * @brief Initialize hardware pin mode and ensure buzzer starts silent.
     */
    virtual void init() = 0;

    /**
     * @brief Non-blocking state update method (to be called in loop).
     */
    virtual void update() = 0;

    /**
     * @brief Play a single beep with custom duration in milliseconds.
     * @param duration_ms Duration of sound in ms (default: 50ms).
     */
    virtual void beep(uint16_t duration_ms = 50) = 0;

    /**
     * @brief Play a predefined buzzer alert pattern.
     * @param pattern Desired pattern from BuzzerPattern enum.
     */
    virtual void play_pattern(BuzzerPattern pattern) = 0;

    /**
     * @brief Immediately silence the buzzer and cancel any ongoing pattern.
     */
    virtual void stop() = 0;

    /**
     * @brief Check whether the buzzer is currently sounding or executing a pattern.
     * @return true if active/playing, false if idle/silent.
     */
    virtual bool is_playing() const = 0;
};

} // namespace ui
