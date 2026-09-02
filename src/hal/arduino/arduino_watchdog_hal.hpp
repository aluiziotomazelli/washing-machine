#pragma once

#include "../interfaces/i_watchdog_hal.hpp"

namespace hal {

/**
 * @class ArduinoWatchdogHAL
 * @brief Concrete Watchdog implementation using AVR hardware Watchdog Timer (avr/wdt.h).
 */
class ArduinoWatchdogHAL : public IWatchdogHAL {
public:
    ArduinoWatchdogHAL() = default;
    ~ArduinoWatchdogHAL() override = default;

    void enable(WatchdogTimeout timeout) override;
    void kick() override;
    void disable() override;
    bool was_reset_by_watchdog() const override;
};

} // namespace hal
