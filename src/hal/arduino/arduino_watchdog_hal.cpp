#include "arduino_watchdog_hal.hpp"

#if defined(__AVR__)
#include <avr/wdt.h>
#include <avr/io.h>

// Mirror variable allocated in .noinit section so the C-runtime doesn't clear it
static uint8_t s_mcusr_mirror __attribute__((section(".noinit")));

// Early initialization in .init3 runs before main() and global constructors.
// This prevents infinite reset loops on bootloaders that don't disable WDT upon reset.
extern "C" void wdt_early_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_early_init(void)
{
    s_mcusr_mirror = MCUSR;
    MCUSR = 0;
    wdt_disable();
}
#endif

namespace hal {

void ArduinoWatchdogHAL::enable(WatchdogTimeout timeout)
{
#if defined(__AVR__)
    uint8_t wdt_val = WDTO_2S;
    switch (timeout) {
    case WatchdogTimeout::TIMEOUT_120MS:
        wdt_val = WDTO_120MS;
        break;
    case WatchdogTimeout::TIMEOUT_250MS:
        wdt_val = WDTO_250MS;
        break;
    case WatchdogTimeout::TIMEOUT_500MS:
        wdt_val = WDTO_500MS;
        break;
    case WatchdogTimeout::TIMEOUT_1S:
        wdt_val = WDTO_1S;
        break;
    case WatchdogTimeout::TIMEOUT_2S:
        wdt_val = WDTO_2S;
        break;
    case WatchdogTimeout::TIMEOUT_4S:
        wdt_val = WDTO_4S;
        break;
    case WatchdogTimeout::TIMEOUT_8S:
        wdt_val = WDTO_8S;
        break;
    }
    wdt_enable(wdt_val);
#else
    (void)timeout;
#endif
}

void ArduinoWatchdogHAL::kick()
{
#if defined(__AVR__)
    wdt_reset();
#endif
}

void ArduinoWatchdogHAL::disable()
{
#if defined(__AVR__)
    wdt_disable();
#endif
}

bool ArduinoWatchdogHAL::was_reset_by_watchdog() const
{
#if defined(__AVR__)
    return (s_mcusr_mirror & (1 << WDRF)) != 0;
#else
    return false;
#endif
}

} // namespace hal
