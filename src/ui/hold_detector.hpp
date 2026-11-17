#pragma once

#include <stdint.h>

namespace ui {

/**
 * @class HoldDetector
 * @brief Detects a one-shot hold gesture from a boolean pressed state and a monotonic timestamp.
 *
 * The detector fires exactly once when the configured hold time is reached. It re-arms only
 * after the pressed input is released, preventing repeated actions while a button remains held.
 */
class HoldDetector
{
public:
    /**
     * @brief Construct a hold detector.
     * @param hold_ms Time the input must remain active before firing.
     */
    explicit HoldDetector(uint32_t hold_ms)
        : hold_ms_(hold_ms)
    {}

    /**
     * @brief Update the detector with the current input state and time.
     * @param pressed True while the gesture is active.
     * @param now_ms Monotonic time in milliseconds.
     * @return True exactly once when the hold threshold is reached.
     */
    bool update(bool pressed, uint32_t now_ms)
    {
        if (!pressed) {
            reset();
            return false;
        }

        if (!pressed_) {
            pressed_ = true;
            start_ms_ = now_ms;
            fired_ = false;
            return false;
        }

        if (fired_ || (now_ms - start_ms_) < hold_ms_) {
            return false;
        }

        fired_ = true;
        return true;
    }

    /**
     * @brief Reset the detector to its idle state.
     */
    void reset()
    {
        pressed_ = false;
        fired_ = false;
    }

private:
    uint32_t hold_ms_;
    uint32_t start_ms_{0};
    bool pressed_{false};
    bool fired_{false};
};

} // namespace ui
