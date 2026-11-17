#pragma once

#include <stdint.h>
#include "../domain/wash_types.hpp"
#include "hold_detector.hpp"
#include "interfaces/i_button.hpp"
#include "interfaces/i_led_panel.hpp"
#include "interfaces/i_buzzer.hpp"
#include "../fsm/wash_cycle_coordinator.hpp"

namespace hal {
class ITimerHAL;
}

namespace persistence {
class ICyclePersistence;
}

namespace ui {

class DiagnosticController;

/**
 * @class PanelController
 * @brief User Interface controller translating button inputs into coordinator commands
 * and synchronizing LED/buzzer state with the running machine state.
 */
class PanelController
{
public:
    static constexpr uint32_t k_diag_trigger_hold_ms{2500};
    static constexpr uint32_t k_stop_trigger_hold_ms{2500};

    /**
     * @brief Construct the UI panel controller with all input/output resources.
     * @param btn_start_pause Start/Pause button input.
     * @param btn_program Program selection button input.
     * @param btn_water_level Water level selection button input.
     * @param btn_softener Softener toggle button input.
     * @param led_panel Visual feedback panel.
     * @param buzzer Audible feedback device.
     * @param coordinator Wash cycle coordinator controlled by the panel.
     * @param diag_ctrl Optional diagnostic controller; nullptr disables diagnostics.
     * @param timer_hal Optional timer used for long-press detection.
     * @param persistence Optional persistence manager used to save user selections.
     */
    PanelController(
        IButton& btn_start_pause,
        IButton& btn_program,
        IButton& btn_water_level,
        IButton& btn_softener,
        ILedPanel& led_panel,
        IBuzzer& buzzer,
        fsm::WashCycleCoordinator& coordinator,
        DiagnosticController* diag_ctrl = nullptr,
        hal::ITimerHAL* timer_hal = nullptr,
        persistence::ICyclePersistence* persistence = nullptr);

    /**
     * @brief Initialize inputs, outputs and restore persisted user preferences.
     */
    void init();

    /**
     * @brief Periodic non-blocking update (button polling and UI synchronization).
     */
    void update();

    /**
     * @brief Get the currently selected wash program.
     * @return Selected WashProgram.
     */
    domain::WashProgram get_selected_program() const { return selected_program_; }

    /**
     * @brief Get the currently selected water level.
     * @return Selected WaterLevel.
     */
    domain::WaterLevel get_selected_level() const { return selected_level_; }

    /**
     * @brief Check whether the softener option is enabled.
     * @return true when softener is enabled.
     */
    bool is_softener_enabled() const { return softener_enabled_; }

    /**
     * @brief Check whether field-service diagnostic mode is active.
     * @return true when the diagnostic controller holds control.
     */
    bool is_diagnostic_active() const;

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
    DiagnosticController* diag_ctrl_{nullptr};
    hal::ITimerHAL* timer_hal_{nullptr};
    persistence::ICyclePersistence* persistence_{nullptr};

    domain::WashProgram selected_program_{domain::WashProgram::RINSE_ONLY};
    domain::WaterLevel selected_level_{domain::WaterLevel::LOW_LEVEL};
    bool softener_enabled_{false};
    HoldDetector diag_hold_{k_diag_trigger_hold_ms};
    HoldDetector stop_hold_{k_stop_trigger_hold_ms};
    bool suppress_start_release_click_{false};

    domain::MachineState prev_state_{domain::MachineState::IDLE};
    domain::WashStage prev_stage_{domain::WashStage::IDLE};
};

} // namespace ui
