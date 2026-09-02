#pragma once

#include <stdint.h>

namespace hal {

/**
 * @enum WatchdogTimeout
 * @brief Standard hardware watchdog timer timeout periods.
 */
enum class WatchdogTimeout : uint8_t {
    TIMEOUT_120MS,
    TIMEOUT_250MS,
    TIMEOUT_500MS,
    TIMEOUT_1S,
    TIMEOUT_2S,
    TIMEOUT_4S,
    TIMEOUT_8S
};

/**
 * @interface IWatchdogHAL
 * @brief Hardware Abstraction Layer interface for hardware watchdog timer.
 */
class IWatchdogHAL {
public:
    virtual ~IWatchdogHAL() = default;

    /**
     * @brief Enable the hardware watchdog timer with the specified timeout.
     * @param timeout Watchdog timeout duration.
     */
    virtual void enable(WatchdogTimeout timeout) = 0;

    /**
     * @brief Reset/refresh the watchdog timer ("kick the dog").
     */
    virtual void kick() = 0;

    /**
     * @brief Disable the hardware watchdog timer.
     */
    virtual void disable() = 0;

    /**
     * @brief Check if the last system reset was triggered by the watchdog timer.
     * @return true if system recovered from a watchdog reset.
     */
    virtual bool was_reset_by_watchdog() const = 0;
};

} // namespace hal
