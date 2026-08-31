#pragma once

#include "../interfaces/i_timer_hal.hpp"

namespace hal {

/**
 * @class ArduinoTimerHAL
 * @brief Concrete Timer implementation using Arduino core timing functions (millis, micros, delay).
 */
class ArduinoTimerHAL : public ITimerHAL {
public:
    ArduinoTimerHAL() = default;
    ~ArduinoTimerHAL() override = default;

    uint32_t get_time_ms() const override;
    uint64_t get_time_us() const override;
    void delay_ms(uint32_t ms) override;
};

} // namespace hal
