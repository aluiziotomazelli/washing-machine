#pragma once

#include <stdint.h>
#include "../../hal/interfaces/i_water_level_sensor.hpp"

namespace ui {

/**
 * @brief Current operational washing stage for visual feedback.
 */
enum class WashStage : uint8_t {
    IDLE = 0,
    WASH,
    RINSE,
    SPIN
};

/**
 * @interface ILedPanel
 * @brief Abstract interface for washing machine visual feedback (supports discrete LEDs or addressable RGB strips).
 */
class ILedPanel {
public:
    virtual ~ILedPanel() = default;

    /**
     * @brief Initialize hardware pins / LED driver.
     */
    virtual void init() = 0;

    /**
     * @brief Non-blocking frame / animation update (to be called in loop).
     */
    virtual void update() = 0;

    /**
     * @brief Control the machine running / power indicator.
     */
    virtual void set_power(bool on) = 0;

    /**
     * @brief Control the softener mode indicator.
     */
    virtual void set_softener(bool enabled) = 0;

    /**
     * @brief Update the wash stage indicator (Wash, Rinse, Spin, or Idle).
     */
    virtual void set_stage(WashStage stage) = 0;

    /**
     * @brief Update the selected water level indicator.
     */
    virtual void set_selected_level(hal::WaterLevel level) = 0;

    /**
     * @brief Set or clear error visual alarm.
     */
    virtual void set_error(bool error) = 0;

    /**
     * @brief De-energize / turn off all LEDs on the panel.
     */
    virtual void turn_off_all() = 0;
};

} // namespace ui
