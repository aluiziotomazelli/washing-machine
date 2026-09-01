#pragma once

#include <stdint.h>
#include "interfaces/i_led_panel.hpp"
#include "../hal/interfaces/i_gpio_hal.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace ui {

/**
 * @brief Pin mapping for discrete indicator LEDs on the washing machine panel.
 * Default values correspond to ATmega328P pinout (A0=14, A1=15, A2=16).
 */
struct DiscreteLedPins {
    uint8_t power{7};        // ledOn
    uint8_t softener{6};     // ledAmaciante
    uint8_t wash{16};        // ledLavar (A2)
    uint8_t rinse{15};       // ledEnxaguar (A1)
    uint8_t spin{14};        // ledCentrifugar (A0)
    uint8_t level_low{13};   // ledNivelB
    uint8_t level_med{12};   // ledNivelM
    uint8_t level_high{255}; // Optional High level LED (255 = not wired)

    DiscreteLedPins() = default;
    DiscreteLedPins(
        uint8_t p, uint8_t s, uint8_t w, uint8_t r, uint8_t sp,
        uint8_t ll, uint8_t lm, uint8_t lh = 255
    )
        : power(p), softener(s), wash(w), rinse(r), spin(sp)
        , level_low(ll), level_med(lm), level_high(lh) {}
};

/**
 * @class DiscreteLedPanel
 * @brief Concrete implementation of ILedPanel driving individual LEDs via GPIO pins.
 */
class DiscreteLedPanel : public ILedPanel {
public:
    DiscreteLedPanel(
        hal::IGpioHAL& gpio_hal,
        hal::ITimerHAL& timer_hal,
        const DiscreteLedPins& pins = DiscreteLedPins{},
        bool active_high = true
    );

    ~DiscreteLedPanel() override = default;

    void init() override;
    void update() override;

    void set_machine_state(domain::MachineState state, domain::MachineError error = domain::MachineError::NONE) override;
    void set_softener(bool enabled) override;
    void set_program(WashProgram program) override;
    void set_stage(WashStage stage) override;
    void set_selected_level(WaterLevel level) override;
    void turn_off_all() override;

private:
    void write_pin(uint8_t pin, bool state);

    hal::IGpioHAL& gpio_hal_;
    hal::ITimerHAL& timer_hal_;
    DiscreteLedPins pins_;
    bool active_high_;
    bool is_initialized_{false};

    WashProgram current_program_{WashProgram::NORMAL_WASH};
    domain::MachineError current_error_{domain::MachineError::NONE};
    bool is_blinking_wash_{false};
    bool is_blinking_error_{false};
    bool is_blinking_power_{false};
    bool blink_state_{false};
    uint32_t last_blink_time_ms_{0};

    static constexpr uint16_t k_blink_interval_ms{500};
};

} // namespace ui
