#include "panel_controller.hpp"
#include "interfaces/i_buzzer.hpp"
#include "diagnostic_controller.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace ui {

PanelController::PanelController(
    IButton& btn_start_pause,
    IButton& btn_program,
    IButton& btn_water_level,
    IButton& btn_softener,
    ILedPanel& led_panel,
    IBuzzer& buzzer,
    fsm::WashCycleCoordinator& coordinator,
    DiagnosticController* diag_ctrl,
    hal::ITimerHAL* timer_hal)
    : btn_start_pause_(btn_start_pause)
    , btn_program_(btn_program)
    , btn_water_level_(btn_water_level)
    , btn_softener_(btn_softener)
    , led_panel_(led_panel)
    , buzzer_(buzzer)
    , coordinator_(coordinator)
    , diag_ctrl_(diag_ctrl)
    , timer_hal_(timer_hal)
{
}

bool PanelController::is_diagnostic_active() const
{
    return diag_ctrl_ != nullptr && diag_ctrl_->is_active();
}

void PanelController::init()
{
    // Default initial selections
    selected_program_ = domain::WashProgram::RINSE_ONLY;
    selected_level_ = domain::WaterLevel::LOW_LEVEL;
    softener_enabled_ = false;

    led_panel_.set_program(selected_program_);
    led_panel_.set_selected_level(selected_level_);
    led_panel_.set_softener(softener_enabled_);
    led_panel_.set_machine_state(domain::MachineState::IDLE);

    buzzer_.beep(50);
}

void PanelController::update()
{
    btn_start_pause_.update();
    btn_program_.update();
    btn_water_level_.update();
    btn_softener_.update();
    buzzer_.update();

    // Check Diagnostic Mode entry trigger (Simultaneous hold of Program + Softener for >= 2.5s in IDLE)
    if (diag_ctrl_ != nullptr && coordinator_.get_state() == domain::MachineState::IDLE && !diag_ctrl_->is_active()) {
        if (btn_program_.is_pressed() && btn_softener_.is_pressed()) {
            uint32_t now = timer_hal_ ? timer_hal_->get_time_ms() : 0;
            if (diag_entry_press_start_ms_ == 0) {
                diag_entry_press_start_ms_ = now;
            } else if (now - diag_entry_press_start_ms_ >= k_diag_trigger_hold_ms) {
                diag_entry_press_start_ms_ = 0;
                // Flush button click buffers before entering diagnostic
                btn_program_.get_last_click();
                btn_softener_.get_last_click();
                btn_start_pause_.get_last_click();
                diag_ctrl_->enter();
            }
        } else {
            diag_entry_press_start_ms_ = 0;
        }
    }

    // If Diagnostic Mode is active, delegate all updates to DiagnosticController
    if (diag_ctrl_ != nullptr && diag_ctrl_->is_active()) {
        diag_ctrl_->update();
        if (!diag_ctrl_->is_active()) {
            // Re-sync UI back to IDLE state upon diagnostic exit
            led_panel_.set_program(selected_program_);
            led_panel_.set_selected_level(selected_level_);
            led_panel_.set_softener(softener_enabled_);
            led_panel_.set_machine_state(domain::MachineState::IDLE);
            prev_state_ = domain::MachineState::IDLE;
        }
        return;
    }

    if (!buzzer_.is_playing()) {
        led_panel_.update();
    }

    ButtonClickType sp_click = btn_start_pause_.get_last_click();
    if (sp_click != ButtonClickType::NONE_CLICK) {
        handle_start_pause_click(sp_click);
    }

    ButtonClickType prog_click = btn_program_.get_last_click();
    if (prog_click == ButtonClickType::CLICK) {
        handle_program_click();
    }

    ButtonClickType level_click = btn_water_level_.get_last_click();
    if (level_click == ButtonClickType::CLICK) {
        handle_water_level_click();
    }

    ButtonClickType soft_click = btn_softener_.get_last_click();
    if (soft_click == ButtonClickType::CLICK) {
        handle_softener_click();
    }

    sync_state_with_coordinator();
}

void PanelController::handle_start_pause_click(ButtonClickType click)
{
    domain::MachineState state = coordinator_.get_state();

    switch (click) {
    case ButtonClickType::CLICK:
        if (state == domain::MachineState::IDLE || state == domain::MachineState::FINISHED) {
            coordinator_.start_cycle(selected_program_, selected_level_, softener_enabled_);
            buzzer_.beep(50);
        }
        else if (state == domain::MachineState::RUNNING) {
            coordinator_.pause_cycle();
            buzzer_.play_pattern(BuzzerPattern::DOUBLE_BEEP);
        }
        else if (state == domain::MachineState::PAUSED || state == domain::MachineState::ERROR) {
            coordinator_.resume_cycle();
            buzzer_.beep(50);
        }
        break;

    case ButtonClickType::LONG_CLICK:
        if (state == domain::MachineState::RUNNING || state == domain::MachineState::PAUSED) {
            coordinator_.advance_step();
            buzzer_.play_pattern(BuzzerPattern::LONG_BEEP);
        }
        break;

    case ButtonClickType::VERY_LONG_CLICK:
        if (state == domain::MachineState::RUNNING || state == domain::MachineState::PAUSED ||
            state == domain::MachineState::ERROR) {
            coordinator_.stop_cycle();
            buzzer_.play_pattern(BuzzerPattern::DOUBLE_BEEP);
        }
        break;

    default:
        break;
    }
}

void PanelController::handle_program_click()
{
    domain::MachineState state = coordinator_.get_state();
    if (state != domain::MachineState::IDLE && state != domain::MachineState::FINISHED) {
        return;
    }

    if (state == domain::MachineState::FINISHED) {
        coordinator_.stop_cycle();
        sync_state_with_coordinator();
    }

    switch (selected_program_) {
    case domain::WashProgram::HEAVY_WASH:
        selected_program_ = domain::WashProgram::NORMAL_WASH;
        break;
    case domain::WashProgram::NORMAL_WASH:
        selected_program_ = domain::WashProgram::RINSE_ONLY;
        break;
    case domain::WashProgram::RINSE_ONLY:
        selected_program_ = domain::WashProgram::SPIN_ONLY;
        break;
    case domain::WashProgram::SPIN_ONLY:
    default:
        selected_program_ = domain::WashProgram::HEAVY_WASH;
        break;
    }

    led_panel_.set_program(selected_program_);
    buzzer_.beep(50);
}

void PanelController::handle_water_level_click()
{
    domain::MachineState state = coordinator_.get_state();
    if (state != domain::MachineState::IDLE && state != domain::MachineState::FINISHED) {
        return;
    }

    if (state == domain::MachineState::FINISHED) {
        coordinator_.stop_cycle();
        sync_state_with_coordinator();
    }

    // Cycle between LOW_LEVEL, MEDIUM_LEVEL, and HIGH_LEVEL
    switch (selected_level_) {
    case domain::WaterLevel::LOW_LEVEL:
        selected_level_ = domain::WaterLevel::MEDIUM_LEVEL;
        break;
    case domain::WaterLevel::MEDIUM_LEVEL:
        selected_level_ = domain::WaterLevel::HIGH_LEVEL;
        break;
    case domain::WaterLevel::HIGH_LEVEL:
    default:
        selected_level_ = domain::WaterLevel::LOW_LEVEL;
        break;
    }

    led_panel_.set_selected_level(selected_level_);
    buzzer_.beep(50);
}

void PanelController::handle_softener_click()
{
    domain::MachineState state = coordinator_.get_state();
    if (state != domain::MachineState::IDLE && state != domain::MachineState::FINISHED) {
        return;
    }

    if (state == domain::MachineState::FINISHED) {
        coordinator_.stop_cycle();
        sync_state_with_coordinator();
    }

    softener_enabled_ = !softener_enabled_;
    led_panel_.set_softener(softener_enabled_);
    buzzer_.beep(50);
}

void PanelController::sync_state_with_coordinator()
{
    domain::MachineState current_state = coordinator_.get_state();
    domain::WashStage current_stage = coordinator_.get_current_stage();

    if (prev_state_ != current_state) {
        led_panel_.set_machine_state(current_state, coordinator_.get_error());

        if (current_state == domain::MachineState::FINISHED) {
            buzzer_.play_pattern(BuzzerPattern::CYCLE_FINISHED);
            led_panel_.set_stage(domain::WashStage::IDLE);
            led_panel_.set_program(selected_program_);
        }
        else if (current_state == domain::MachineState::ERROR) {
            buzzer_.play_pattern(BuzzerPattern::ERROR_ALARM);
        }
        else if (current_state == domain::MachineState::IDLE) {
            led_panel_.set_stage(domain::WashStage::IDLE);
            led_panel_.set_program(selected_program_);
        }
        else if (current_state == domain::MachineState::RUNNING) {
            led_panel_.set_stage(current_stage);
        }

        prev_state_ = current_state;
        prev_stage_ = current_stage;
    }
    else if (current_state == domain::MachineState::RUNNING && prev_stage_ != current_stage) {
        led_panel_.set_stage(current_stage);
        prev_stage_ = current_stage;
    }
}

} // namespace ui
