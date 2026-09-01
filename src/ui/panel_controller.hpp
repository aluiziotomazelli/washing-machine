#pragma once

#include <stdint.h>
#include "../domain/wash_types.hpp"
#include "interfaces/i_button.hpp"
#include "interfaces/i_led_panel.hpp"
#include "interfaces/i_buzzer.hpp"
#include "../fsm/wash_cycle_coordinator.hpp"

namespace ui {

/**
 * @class PanelController
 * @brief User Interface controller translating button inputs into coordinator commands
 * and synchronizing LED/buzzer state with the running machine state.
 */
class PanelController {
public:
    PanelController(
        IButton& btn_start_pause,
        IButton& btn_program,
        IButton& btn_water_level,
        IButton& btn_softener,
        ILedPanel& led_panel,
        IBuzzer& buzzer,
        fsm::WashCycleCoordinator& coordinator
    );

    void init();
    void update();

    domain::WashProgram get_selected_program() const { return selected_program_; }
    domain::WaterLevel get_selected_level() const { return selected_level_; }
    bool is_softener_enabled() const { return softener_enabled_; }

private:
    void handle_start_pause_click(ButtonClickType click);
    void handle_program_click();
    void handle_water_level_click();
    void handle_softener_click();
    void sync_state_with_coordinator();

    IButton& btn_start_pause_;
    IButton& btn_program_;
    IButton& btn_water_level_;
    IButton& btn_softener_;
    ILedPanel& led_panel_;
    IBuzzer& buzzer_;
    fsm::WashCycleCoordinator& coordinator_;

    domain::WashProgram selected_program_{domain::WashProgram::NORMAL_WASH};
    domain::WaterLevel selected_level_{domain::WaterLevel::LOW_LEVEL};
    bool softener_enabled_{false};

    domain::MachineState prev_state_{domain::MachineState::IDLE};
    domain::WashStage prev_stage_{domain::WashStage::IDLE};
};

} // namespace ui
