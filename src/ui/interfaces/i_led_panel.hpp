#pragma once

#include <stdint.h>
#include "../../domain/wash_types.hpp"

namespace ui {

using domain::MachineState;
using domain::WashProgram;
using domain::WashStage;
using domain::WaterLevel;

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
     * @brief Non-blocking frame / animation update (to be called periodically in loop).
     */
    virtual void update() = 0;

    /**
     * @brief Update the machine macro state for visual feedback (IDLE, RUNNING, PAUSED, ERROR, FINISHED).
     */
    virtual void set_machine_state(domain::MachineState state) = 0;

    /**
     * @brief Control the softener mode indicator.
     */
    virtual void set_softener(bool enabled) = 0;

    /**
     * @brief Set the visual program selection during IDLE mode (e.g. blinking for heavy wash).
     */
    virtual void set_program(WashProgram program) = 0;

    /**
     * @brief Update the wash stage indicator during execution (Wash, Rinse, Spin, or Idle).
     */
    virtual void set_stage(WashStage stage) = 0;

    /**
     * @brief Update the selected water level indicator.
     */
    virtual void set_selected_level(WaterLevel level) = 0;

    /**
     * @brief Turn off all indicators immediately.
     */
    virtual void turn_off_all() = 0;
};

} // namespace ui
