#pragma once

#include <stdint.h>
#include "interfaces/i_water_level_sensor.hpp"
#include "interfaces/i_gpio_hal.hpp"
#include "interfaces/i_timer_hal.hpp"

namespace hal {

/**
 * @brief Electrical contact type for pressure switch contacts.
 */
enum class ContactType : uint8_t {
    NORMALLY_OPEN,   // NO: Active when circuit closes to GND (Logic LEVEL_LOW)
    NORMALLY_CLOSED  // NC: Active when circuit opens under pressure (Logic LEVEL_HIGH)
};

/**
 * @brief Configuration for an individual water level switch contact pin.
 */
struct LevelSensorPin {
    uint8_t pin{255};                                  //!< GPIO pin number (255 = contact not wired).
    ContactType contact{ContactType::NORMALLY_OPEN};   //!< Electrical contact type of the switch.

    LevelSensorPin() = default;
    LevelSensorPin(uint8_t p, ContactType c) : pin(p), contact(c) {}
};

/**
 * @brief Configuration parameters for a multi-contact electromechanical pressure switch.
 */
struct PressureSwitchConfig {
    LevelSensorPin low{10, ContactType::NORMALLY_CLOSED};  //!< Low level contact (31-32 NC, D10).
    LevelSensorPin medium{11, ContactType::NORMALLY_OPEN}; //!< Medium level contact (11-13 NO, D11).
    LevelSensorPin high{255, ContactType::NORMALLY_OPEN};  //!< High level contact (21-23 NO; 255 disables it by default).
    uint16_t debounce_ms{100};                             //!< Hydraulic stabilization window.

    PressureSwitchConfig() = default;
    PressureSwitchConfig(LevelSensorPin l, LevelSensorPin m, LevelSensorPin h = LevelSensorPin{255, ContactType::NORMALLY_OPEN}, uint16_t db = 100)
        : low(l), medium(m), high(h), debounce_ms(db) {}
};

/**
 * @class PressureSwitchSensor
 * @brief Platform-agnostic driver for electromechanical washing machine pressure switches.
 * 
 * Supports mixed NC/NO contacts and implements software hydraulic debouncing to filter
 * water sloshing waves during filling and agitation.
 */
class PressureSwitchSensor : public IWaterLevelSensor {
public:
    /**
     * @brief Construct a pressure switch reader for three level contacts.
     * @param gpio_hal GPIO abstraction used to sample the contact pins.
     * @param timer_hal Timer abstraction used for hydraulic debouncing.
     * @param config Pin/contact mapping and debounce window (defaults to the reference board map).
     *
     * @note On the ATmega328P reference build the high-level contact is wired to D12
     *       through config::k_pressure_switch_high_pin. The default Pin255 value disables
     *       the high contact when no pin is supplied.
     */
    PressureSwitchSensor(
        IGpioHAL& gpio_hal,
        ITimerHAL& timer_hal,
        const PressureSwitchConfig& config = PressureSwitchConfig{}
    );

    ~PressureSwitchSensor() override = default;

    /** @copydoc IWaterLevelSensor::init */
    void init() override;
    /** @copydoc IWaterLevelSensor::update */
    void update() override;

    /** @copydoc IWaterLevelSensor::is_level_reached */
    bool is_level_reached(WaterLevel target) const override;
    /** @copydoc IWaterLevelSensor::get_current_level */
    WaterLevel get_current_level() const override;
    /** @copydoc IWaterLevelSensor::is_empty */
    bool is_empty() const override;

private:
    IGpioHAL& gpio_hal_;
    ITimerHAL& timer_hal_;
    PressureSwitchConfig config_;
    bool is_initialized_{false};

    // Stable debounced states
    bool stable_low_{false};
    bool stable_med_{false};
    bool stable_high_{false};

    // Raw last sampled states & change timestamps for debouncing
    bool last_raw_low_{false};
    bool last_raw_med_{false};
    bool last_raw_high_{false};

    uint32_t last_change_time_low_{0};
    uint32_t last_change_time_med_{0};
    uint32_t last_change_time_high_{0};

    bool sample_raw_active(const LevelSensorPin& pin_cfg) const;
    void update_contact_debounce(
        const LevelSensorPin& pin_cfg,
        bool& stable_state,
        bool& last_raw,
        uint32_t& last_change_time,
        uint32_t now
    );
};

} // namespace hal
