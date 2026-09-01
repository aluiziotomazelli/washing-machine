#include "pressure_switch_sensor.hpp"

namespace hal {

PressureSwitchSensor::PressureSwitchSensor(
    IGpioHAL& gpio_hal,
    ITimerHAL& timer_hal,
    const PressureSwitchConfig& config
)
    : gpio_hal_(gpio_hal)
    , timer_hal_(timer_hal)
    , config_(config)
    , is_initialized_(false)
    , stable_low_(false)
    , stable_med_(false)
    , stable_high_(false)
    , last_raw_low_(false)
    , last_raw_med_(false)
    , last_raw_high_(false)
    , last_change_time_low_(0)
    , last_change_time_med_(0)
    , last_change_time_high_(0)
{
}

void PressureSwitchSensor::init()
{
    if (config_.low.pin != 255) {
        gpio_hal_.set_mode(config_.low.pin, GpioMode::MODE_INPUT_PULLUP);
    }
    if (config_.medium.pin != 255) {
        gpio_hal_.set_mode(config_.medium.pin, GpioMode::MODE_INPUT_PULLUP);
    }
    if (config_.high.pin != 255) {
        gpio_hal_.set_mode(config_.high.pin, GpioMode::MODE_INPUT_PULLUP);
    }

    uint32_t now = timer_hal_.get_time_ms();
    last_raw_low_ = stable_low_ = sample_raw_active(config_.low);
    last_raw_med_ = stable_med_ = sample_raw_active(config_.medium);
    last_raw_high_ = stable_high_ = sample_raw_active(config_.high);

    last_change_time_low_ = now;
    last_change_time_med_ = now;
    last_change_time_high_ = now;

    is_initialized_ = true;
}

void PressureSwitchSensor::update()
{
    if (!is_initialized_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    update_contact_debounce(config_.low, stable_low_, last_raw_low_, last_change_time_low_, now);
    update_contact_debounce(config_.medium, stable_med_, last_raw_med_, last_change_time_med_, now);
    update_contact_debounce(config_.high, stable_high_, last_raw_high_, last_change_time_high_, now);
}

bool PressureSwitchSensor::is_level_reached(WaterLevel target) const
{
    if (!is_initialized_) {
        return false;
    }

    switch (target) {
    case WaterLevel::EMPTY:
        return true;
    case WaterLevel::LOW_LEVEL:
        return stable_low_ || stable_med_ || stable_high_;
    case WaterLevel::MEDIUM_LEVEL:
        return stable_med_ || stable_high_;
    case WaterLevel::HIGH_LEVEL:
        return stable_high_;
    default:
        return false;
    }
}

WaterLevel PressureSwitchSensor::get_current_level() const
{
    if (!is_initialized_) {
        return WaterLevel::EMPTY;
    }

    if (stable_high_) {
        return WaterLevel::HIGH_LEVEL;
    }
    if (stable_med_) {
        return WaterLevel::MEDIUM_LEVEL;
    }
    if (stable_low_) {
        return WaterLevel::LOW_LEVEL;
    }
    return WaterLevel::EMPTY;
}

bool PressureSwitchSensor::is_empty() const
{
    if (!is_initialized_) {
        return true;
    }

    return !stable_low_ && !stable_med_ && !stable_high_;
}

bool PressureSwitchSensor::sample_raw_active(const LevelSensorPin& pin_cfg) const
{
    if (pin_cfg.pin == 255) {
        return false;
    }

    GpioLevel level = gpio_hal_.get_level(pin_cfg.pin);

    if (pin_cfg.contact == ContactType::NORMALLY_CLOSED) {
        // NC opens when activated -> Level goes HIGH
        return level == GpioLevel::LEVEL_HIGH;
    } else {
        // NO closes to GND when activated -> Level goes LOW
        return level == GpioLevel::LEVEL_LOW;
    }
}

void PressureSwitchSensor::update_contact_debounce(
    const LevelSensorPin& pin_cfg,
    bool& stable_state,
    bool& last_raw,
    uint32_t& last_change_time,
    uint32_t now
)
{
    if (pin_cfg.pin == 255) {
        stable_state = false;
        return;
    }

    bool raw = sample_raw_active(pin_cfg);

    if (raw != last_raw) {
        last_raw = raw;
        last_change_time = now;
    } else if (now - last_change_time >= config_.debounce_ms) {
        stable_state = raw;
    }
}

} // namespace hal
