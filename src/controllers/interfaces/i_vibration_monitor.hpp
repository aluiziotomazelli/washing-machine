#pragma once

#include <stdint.h>
#include "../../hal/interfaces/i_accelerometer.hpp"

namespace controllers {

/**
 * @interface IVibrationMonitor
 * @brief Interface for mechanical vibration tracking, motion detection, and unbalance protection.
 */
class IVibrationMonitor {
public:
    virtual ~IVibrationMonitor() = default;

    /**
     * @brief Initialize or re-initialize monitor state.
     */
    virtual void init() = 0;

    /**
     * @brief Periodic non-blocking processing update.
     * Samples accelerometer at configured interval and computes vibration envelope.
     */
    virtual void update() = 0;

    /**
     * @brief Current 3D peak-to-peak vibration envelope metric.
     * @return Vibration index (delta X + delta Y + delta Z over 200 ms window).
     */
    virtual uint16_t get_vibration() const = 0;

    /**
     * @brief Most recent raw 3-axis accelerometer sample.
     */
    virtual const hal::Vector3& get_last_sample() const = 0;

    /**
     * @brief Whether physical movement is detected (agitation or drum spinning).
     * @return true if vibration exceeds motion threshold.
     */
    virtual bool is_in_motion() const = 0;

    /**
     * @brief Whether vibration is in the warning zone (perceptible unbalance).
     * @return true if vibration exceeds warning threshold.
     */
    virtual bool is_warning() const = 0;

    /**
     * @brief Whether critical unbalance trip condition was reached.
     * @return true if vibration exceeded sustained limit (> 1s) or immediate shock threshold.
     */
    virtual bool is_critical_unbalance() const = 0;

    /**
     * @brief Reset trip and warning flags, and clear sampling buffer.
     */
    virtual void reset() = 0;
};

} // namespace controllers
