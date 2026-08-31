#pragma once

#include <stdint.h>

namespace hal {

/**
 * @interface ITimerHAL
 * @brief Hardware Abstraction Layer interface for system time and timer services.
 */
class ITimerHAL {
public:
    virtual ~ITimerHAL() = default;

    /**
     * @brief Get elapsed time in milliseconds since system startup.
     * @return System time in milliseconds.
     */
    virtual uint32_t get_time_ms() const = 0;

    /**
     * @brief Get elapsed time in microseconds since system startup.
     * @return System time in microseconds.
     */
    virtual uint64_t get_time_us() const = 0;

    /**
     * @brief Busy-wait delay in milliseconds.
     * @param ms Duration in milliseconds to wait.
     */
    virtual void delay_ms(uint32_t ms) = 0;
};

} // namespace hal
