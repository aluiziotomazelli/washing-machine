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
    uint8_t pin{255};
    ContactType contact{ContactType::NORMALLY_OPEN};

    LevelSensorPin() = default;
    LevelSensorPin(uint8_t p, ContactType c) : pin(p), contact(c) {}
};

/**
 * @brief Configuration parameters for a multi-contact electromechanical pressure switch.
 */
struct PressureSwitchConfig {
    LevelSensorPin low{10, ContactType::NORMALLY_CLOSED};  // 31-32: NC (Low level, Pin 10)
    LevelSensorPin medium{11, ContactType::NORMALLY_OPEN}; // 11-13: NO (Medium level, Pin 11)
    LevelSensorPin high{255, ContactType::NORMALLY_OPEN};  // 21-23: NO (High level, 255 on ATmega328P due to pin 12 being ledNivelM)
    uint16_t debounce_ms{100};                             // Hydraulic stabilization window

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
    PressureSwitchSensor(
        IGpioHAL& gpio_hal,
        ITimerHAL& timer_hal,
        const PressureSwitchConfig& config = PressureSwitchConfig{}
    );

    ~PressureSwitchSensor() override = default;

    void init() override;
    void update() override;

    bool is_level_reached(WaterLevel target) const override;
    WaterLevel get_current_level() const override;
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
