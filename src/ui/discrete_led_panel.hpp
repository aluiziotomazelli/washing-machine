#pragma once

#include <stdint.h>
#include "interfaces/i_led_panel.hpp"
#include "../hal/interfaces/i_gpio_hal.hpp"

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
        const DiscreteLedPins& pins = DiscreteLedPins{},
        bool active_high = true
    );

    ~DiscreteLedPanel() override = default;

    void init() override;
    void update() override;

    void set_power(bool on) override;
    void set_softener(bool enabled) override;
    void set_stage(WashStage stage) override;
    void set_selected_level(hal::WaterLevel level) override;
    void set_error(bool error) override;
    void turn_off_all() override;

private:
    hal::IGpioHAL& gpio_hal_;
    DiscreteLedPins pins_;
    bool active_high_;
    bool is_initialized_{false};

    void write_pin(uint8_t pin, bool on);
};

} // namespace ui
