#include "panel_controller.hpp"

namespace ui {

PanelController::PanelController(
    IButton& btn_start_pause,
    IButton& btn_program,
    IButton& btn_water_level,
    IButton& btn_softener,
    ILedPanel& led_panel,
    IBuzzer& buzzer,
    fsm::WashCycleCoordinator& coordinator
)
    : btn_start_pause_(btn_start_pause)
    , btn_program_(btn_program)
    , btn_water_level_(btn_water_level)
    , btn_softener_(btn_softener)
    , led_panel_(led_panel)
    , buzzer_(buzzer)
    , coordinator_(coordinator)
{
}

void PanelController::init()
{
    // Default initial selections
    selected_program_ = domain::WashProgram::NORMAL_WASH;
    selected_level_ = domain::WaterLevel::LOW_LEVEL;
    softener_enabled_ = false;

    led_panel_.set_program(selected_program_);
    led_panel_.set_selected_level(selected_level_);
    led_panel_.set_softener(softener_enabled_);
    led_panel_.set_stage(domain::WashStage::IDLE);
    led_panel_.set_power(true);

    buzzer_.beep(50);
}

void PanelController::update()
{
    btn_start_pause_.update();
    btn_program_.update();
    btn_water_level_.update();
    btn_softener_.update();
    buzzer_.update();
    led_panel_.update();

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
        } else if (state == domain::MachineState::RUNNING) {
            coordinator_.pause_cycle();
            buzzer_.play_pattern(BuzzerPattern::DOUBLE_BEEP);
        } else if (state == domain::MachineState::PAUSED) {
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
        if (state == domain::MachineState::RUNNING || state == domain::MachineState::PAUSED) {
            coordinator_.stop_cycle();
            buzzer_.beep(100);
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

    switch (selected_program_) {
    case domain::WashProgram::NORMAL_WASH:
        selected_program_ = domain::WashProgram::HEAVY_WASH;
        break;
    case domain::WashProgram::HEAVY_WASH:
        selected_program_ = domain::WashProgram::RINSE_ONLY;
        break;
    case domain::WashProgram::RINSE_ONLY:
        selected_program_ = domain::WashProgram::SPIN_ONLY;
        break;
    case domain::WashProgram::SPIN_ONLY:
    default:
        selected_program_ = domain::WashProgram::NORMAL_WASH;
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

    // Toggle between LOW_LEVEL and MEDIUM_LEVEL (HIGH_LEVEL is disabled on this PCB)
    if (selected_level_ == domain::WaterLevel::LOW_LEVEL) {
        selected_level_ = domain::WaterLevel::MEDIUM_LEVEL;
    } else {
        selected_level_ = domain::WaterLevel::LOW_LEVEL;
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

    softener_enabled_ = !softener_enabled_;
    led_panel_.set_softener(softener_enabled_);
    buzzer_.beep(50);
}

void PanelController::sync_state_with_coordinator()
{
    domain::MachineState current_state = coordinator_.get_state();
    domain::WashStage current_stage = coordinator_.get_current_stage();

    if (prev_state_ != current_state) {
        if (current_state == domain::MachineState::FINISHED) {
            buzzer_.play_pattern(BuzzerPattern::CYCLE_FINISHED);
            led_panel_.set_stage(domain::WashStage::IDLE);
            led_panel_.set_program(selected_program_);
            led_panel_.set_power(true);
        } else if (current_state == domain::MachineState::ERROR) {
            buzzer_.play_pattern(BuzzerPattern::ERROR_ALARM);
            led_panel_.set_error(true);
        } else if (current_state == domain::MachineState::IDLE) {
            led_panel_.set_stage(domain::WashStage::IDLE);
            led_panel_.set_program(selected_program_);
            led_panel_.set_power(true);
        } else if (current_state == domain::MachineState::RUNNING) {
            led_panel_.set_power(true);
            led_panel_.set_stage(current_stage);
        }

        if (prev_state_ == domain::MachineState::ERROR && current_state != domain::MachineState::ERROR) {
            led_panel_.set_error(false);
        }

        prev_state_ = current_state;
        prev_stage_ = current_stage;
    } else if (current_state == domain::MachineState::RUNNING && prev_stage_ != current_stage) {
        led_panel_.set_stage(current_stage);
        prev_stage_ = current_stage;
    }
}

} // namespace ui
