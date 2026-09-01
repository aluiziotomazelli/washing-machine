#pragma once

#include <stdint.h>
#include "interfaces/i_buzzer.hpp"
#include "../hal/interfaces/i_gpio_hal.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace ui {

/**
 * @class Buzzer
 * @brief Platform-agnostic non-blocking buzzer driver supporting passive piezo transducers (tone generation)
 *        as well as active buzzers.
 */
class Buzzer : public IBuzzer {
public:
    Buzzer(
        hal::IGpioHAL& gpio_hal,
        hal::ITimerHAL& timer_hal,
        uint8_t pin,
        uint16_t frequency_hz = 3000
    );

    ~Buzzer() override = default;

    void init() override;
    void update() override;
    void beep(uint16_t duration_ms = 50) override;
    void play_pattern(BuzzerPattern pattern) override;
    void stop() override;
    bool is_playing() const override;

private:
    hal::IGpioHAL& gpio_hal_;
    hal::ITimerHAL& timer_hal_;
    uint8_t pin_;
    uint16_t frequency_hz_;
    bool is_initialized_{false};

    bool is_playing_{false};
    bool is_sounding_{false};
    bool continuous_{false};

    uint16_t on_time_ms_{0};
    uint16_t off_time_ms_{0};
    uint8_t remaining_beeps_{0};
    uint32_t phase_start_time_ms_{0};

    void apply_sound(bool on);
    void start_sequence(uint16_t on_ms, uint16_t off_ms, uint8_t count, bool continuous = false);
};

} // namespace ui
