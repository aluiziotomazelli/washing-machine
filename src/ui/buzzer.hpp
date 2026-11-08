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
    /**
     * @brief Construct a buzzer driver bound to a GPIO pin.
     * @param gpio_hal GPIO/tone abstraction used to drive the buzzer.
     * @param timer_hal Timer abstraction used for pattern timing.
     * @param pin Hardware pin connected to the buzzer.
     * @param frequency_hz Tone frequency for passive piezo transducers (default 3000 Hz).
     */
    Buzzer(
        hal::IGpioHAL& gpio_hal,
        hal::ITimerHAL& timer_hal,
        uint8_t pin,
        uint16_t frequency_hz = 3000
    );

    ~Buzzer() override = default;

    /** @copydoc IBuzzer::init */
    void init() override;
    /** @copydoc IBuzzer::update */
    void update() override;
    /** @copydoc IBuzzer::beep */
    void beep(uint16_t duration_ms = 50) override;
    /** @copydoc IBuzzer::play_pattern */
    void play_pattern(BuzzerPattern pattern) override;
    /** @copydoc IBuzzer::stop */
    void stop() override;
    /** @copydoc IBuzzer::is_playing */
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
