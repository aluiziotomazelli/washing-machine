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
    uint8_t power{7};        //!< Power indicator (ledOn).
    uint8_t softener{6};     //!< Softener indicator (ledAmaciante).
    uint8_t wash{16};        //!< Wash stage indicator (ledLavar, A2).
    uint8_t rinse{15};       //!< Rinse stage indicator (ledEnxaguar, A1).
    uint8_t spin{14};        //!< Spin stage indicator (ledCentrifugar, A0).
    uint8_t level_low{13};   //!< Low level indicator (ledNivelB).
    uint8_t level_med{12};   //!< Medium level indicator (ledNivelM).
    uint8_t level_high{255}; //!< Optional high level indicator (255 = not wired).

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
    /**
     * @brief Construct a discrete LED panel.
     * @param gpio_hal GPIO abstraction used to drive the LEDs.
     * @param timer_hal Timer abstraction used for blink timing.
     * @param pins Pin mapping for each indicator LED.
     * @param active_high true when an LED illuminates on a HIGH level.
     *
     * @note The default level_med pin is D12 (ledNivelM) on the original board,
     *       which conflicts with config::k_pressure_switch_high_pin on the current
     *       reference build. Use StripLedPanel for that build.
     */
    DiscreteLedPanel(
        hal::IGpioHAL& gpio_hal,
        hal::ITimerHAL& timer_hal,
        const DiscreteLedPins& pins = DiscreteLedPins{},
        bool active_high = true
    );

    ~DiscreteLedPanel() override = default;

    /** @copydoc ILedPanel::init */
    void init() override;
    /** @copydoc ILedPanel::update */
    void update() override;

    /** @copydoc ILedPanel::set_machine_state */
    void set_machine_state(domain::MachineState state, domain::MachineError error = domain::MachineError::NONE) override;
    /** @copydoc ILedPanel::set_softener */
    void set_softener(bool enabled) override;
    /** @copydoc ILedPanel::set_program */
    void set_program(WashProgram program) override;
    /** @copydoc ILedPanel::set_stage */
    void set_stage(WashStage stage) override;
    /** @copydoc ILedPanel::set_selected_level */
    void set_selected_level(WaterLevel level) override;
    /** @copydoc ILedPanel::turn_off_all */
    void turn_off_all() override;
    /** @copydoc ILedPanel::show_diagnostic */
    void show_diagnostic(DiagnosticStep step, uint16_t raw_value = 0, bool status_ok = true) override;

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
