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

    /** @copydoc IWatchdogHAL::enable */
    void enable(WatchdogTimeout timeout) override;
    /** @copydoc IWatchdogHAL::kick */
    void kick() override;
    /** @copydoc IWatchdogHAL::disable */
    void disable() override;
    /** @copydoc IWatchdogHAL::was_reset_by_watchdog */
    bool was_reset_by_watchdog() const override;
};

} // namespace hal
