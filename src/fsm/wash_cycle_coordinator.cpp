#include "wash_cycle_coordinator.hpp"

namespace fsm {

WashCycleCoordinator::WashCycleCoordinator(
    hal::ITimerHAL& timer_hal,
    controllers::FillController& fill_ctrl,
    controllers::Agitator& agitator,
    controllers::DrainController& drain_ctrl,
    controllers::SpinController& spin_ctrl,
    const CoordinatorConfig& config)
    : timer_hal_(timer_hal)
    , fill_ctrl_(fill_ctrl)
    , agitator_(agitator)
    , drain_ctrl_(drain_ctrl)
    , spin_ctrl_(spin_ctrl)
    , config_(config)
{
}

void WashCycleCoordinator::init()
{
    stop_cycle();
}

void WashCycleCoordinator::start_cycle(WashProgram program, WaterLevel level, bool softener_enabled)
{
    stop_active_process();

    program_ = program;
    level_ = level;
    softener_enabled_ = softener_enabled;
    state_ = MachineState::RUNNING;
    step_index_ = 0;
    in_rinse_subcycle_ = false;

    plan_next_step();
}

void WashCycleCoordinator::pause_cycle()
{
    if (state_ != MachineState::RUNNING) {
        return;
    }

    state_ = MachineState::PAUSED;
    uint32_t now = timer_hal_.get_time_ms();

    if (is_soaking_) {
        soak_elapsed_before_pause_ms_ += (now - step_start_ms_);
    }

    if (fill_ctrl_.is_active()) {
        fill_ctrl_.pause();
    }
    if (agitator_.is_active()) {
        agitator_.pause();
    }
    if (drain_ctrl_.is_active()) {
        drain_ctrl_.pause();
    }
    if (spin_ctrl_.is_active()) {
        spin_ctrl_.pause();
    }
}

void WashCycleCoordinator::resume_cycle()
{
    if (state_ != MachineState::PAUSED) {
        return;
    }

    state_ = MachineState::RUNNING;
    step_start_ms_ = timer_hal_.get_time_ms();

    if (fill_ctrl_.is_paused()) {
        fill_ctrl_.resume();
    }
    if (agitator_.is_paused()) {
        agitator_.resume();
    }
    if (drain_ctrl_.is_paused()) {
        drain_ctrl_.resume();
    }
    if (spin_ctrl_.is_paused()) {
        spin_ctrl_.resume();
    }
}

void WashCycleCoordinator::advance_step()
{
    if (state_ != MachineState::RUNNING && state_ != MachineState::PAUSED) {
        return;
    }

    stop_active_process();
    state_ = MachineState::RUNNING;
    step_index_++;
    plan_next_step();
}

void WashCycleCoordinator::stop_cycle()
{
    stop_active_process();
    state_ = MachineState::IDLE;
    current_stage_ = WashStage::IDLE;
    current_step_ = CycleStep::NONE;
    step_index_ = 0;
    in_rinse_subcycle_ = false;
}

void WashCycleCoordinator::stop_active_process()
{
    fill_ctrl_.stop();
    agitator_.stop();
    drain_ctrl_.stop();
    spin_ctrl_.stop();

    is_soaking_ = false;
    is_settling_ = false;
    soak_elapsed_before_pause_ms_ = 0;
}

void WashCycleCoordinator::trigger_error()
{
    stop_active_process();
    state_ = MachineState::ERROR;
}

void WashCycleCoordinator::update()
{
    if (state_ != MachineState::RUNNING) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();

    if (is_settling_) {
        if (now - step_start_ms_ >= config_.stage_settle_ms) {
            is_settling_ = false;
            step_index_++;
            plan_next_step();
        }
        return;
    }

    if (is_soaking_) {
        uint32_t total = soak_elapsed_before_pause_ms_ + (now - step_start_ms_);
        if (total >= soak_duration_ms_) {
            is_soaking_ = false;
            step_index_++;
            plan_next_step();
        }
        return;
    }

    if (fill_ctrl_.is_active()) {
        fill_ctrl_.update();
        if (fill_ctrl_.has_error()) {
            trigger_error();
        }
        else if (fill_ctrl_.is_finished()) {
            // Water filled! Settle water before motor starts
            is_settling_ = true;
            current_step_ = CycleStep::SETTLE_PAUSE;
            step_start_ms_ = now;
        }
        return;
    }

    if (agitator_.is_active()) {
        agitator_.update();
        if (agitator_.is_finished()) {
            step_index_++;
            plan_next_step();
        }
        return;
    }

    if (drain_ctrl_.is_active()) {
        drain_ctrl_.update();
        if (drain_ctrl_.has_error()) {
            trigger_error();
        }
        else if (drain_ctrl_.is_finished()) {
            step_index_++;
            plan_next_step();
        }
        return;
    }

    if (spin_ctrl_.is_active()) {
        spin_ctrl_.update();
        if (spin_ctrl_.is_finished()) {
            step_index_++;
            plan_next_step();
        }
        return;
    }
}

void WashCycleCoordinator::execute_step(CycleStep step)
{
    stop_active_process();
    current_step_ = step;
    step_start_ms_ = timer_hal_.get_time_ms();

    switch (step) {
    case CycleStep::FILL_MAIN:
        fill_ctrl_.start(level_, false);
        break;

    case CycleStep::FILL_SOFTENER:
        fill_ctrl_.start(level_, true);
        break;

    case CycleStep::AGITATE_GENTLE:
        if (program_ == WashProgram::HEAVY_WASH && !in_rinse_subcycle_) {
            agitator_.start(config_.heavy_wash_agitate1_sec, 300, 300);
        }
        else {
            agitator_.start(config_.double_rinse_2_agitate_sec, 300, 300);
        }
        break;

    case CycleStep::AGITATE_NORMAL:
        if (program_ == WashProgram::NORMAL_WASH && !in_rinse_subcycle_) {
            agitator_.start(config_.normal_wash_agitate_sec, 300, 200);
        }
        else if (program_ == WashProgram::HEAVY_WASH && !in_rinse_subcycle_) {
            agitator_.start(config_.heavy_wash_agitate2_sec, 300, 200);
        }
        else if (!softener_enabled_) {
            agitator_.start(config_.single_rinse_agitate_sec, 300, 200);
        }
        else if (step_index_ == 1) {
            agitator_.start(config_.double_rinse_1_agitate_sec, 300, 200);
        }
        else {
            agitator_.start(config_.double_rinse_2_agitate_post_sec, 300, 200);
        }
        break;

    case CycleStep::SOAK:
        is_soaking_ = true;
        soak_elapsed_before_pause_ms_ = 0;
        if (program_ == WashProgram::HEAVY_WASH && !in_rinse_subcycle_) {
            soak_duration_ms_ = config_.heavy_wash_soak_sec * 1000;
        }
        else {
            soak_duration_ms_ = config_.double_rinse_2_soak_sec * 1000;
        }
        break;

    case CycleStep::DRAIN:
        drain_ctrl_.start();
        break;

    case CycleStep::SPIN_INTERMEDIATE:
        spin_ctrl_.start(level_, config_.double_rinse_interm_spin_sec);
        break;

    case CycleStep::SPIN_FINAL:
        spin_ctrl_.start(level_, config_.final_spin_sec);
        break;

    default:
        break;
    }
}

void WashCycleCoordinator::plan_next_step()
{
    switch (program_) {
    case WashProgram::SPIN_ONLY:
        current_stage_ = WashStage::SPIN;
        switch (step_index_) {
        case 0:
            execute_step(CycleStep::DRAIN);
            break;
        case 1:
            execute_step(CycleStep::SPIN_FINAL);
            break;
        default:
            stop_active_process();
            state_ = MachineState::FINISHED;
            current_stage_ = WashStage::IDLE;
            current_step_ = CycleStep::FINISHED;
            break;
        }
        break;

    case WashProgram::RINSE_ONLY:
        if (!softener_enabled_) {
            // Single Rinse Recipe
            switch (step_index_) {
            case 0:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::FILL_MAIN);
                break;
            case 1:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::AGITATE_NORMAL);
                break;
            case 2:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::DRAIN);
                break;
            case 3:
                current_stage_ = WashStage::SPIN;
                execute_step(CycleStep::SPIN_FINAL);
                break;
            default:
                stop_active_process();
                state_ = MachineState::FINISHED;
                current_stage_ = WashStage::IDLE;
                current_step_ = CycleStep::FINISHED;
                break;
            }
        }
        else {
            // Double Rinse Recipe with Softener
            switch (step_index_) {
            case 0:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::FILL_MAIN);
                break;
            case 1:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::AGITATE_NORMAL);
                break;
            case 2:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::DRAIN);
                break;
            case 3:
                current_stage_ = WashStage::SPIN;
                execute_step(CycleStep::SPIN_INTERMEDIATE);
                break;
            case 4:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::FILL_SOFTENER);
                break;
            case 5:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::AGITATE_GENTLE);
                break;
            case 6:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::SOAK);
                break;
            case 7:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::AGITATE_NORMAL);
                break;
            case 8:
                current_stage_ = WashStage::RINSE;
                execute_step(CycleStep::DRAIN);
                break;
            case 9:
                current_stage_ = WashStage::SPIN;
                execute_step(CycleStep::SPIN_FINAL);
                break;
            default:
                stop_active_process();
                state_ = MachineState::FINISHED;
                current_stage_ = WashStage::IDLE;
                current_step_ = CycleStep::FINISHED;
                break;
            }
        }
        break;

    case WashProgram::NORMAL_WASH:
        if (!in_rinse_subcycle_) {
            current_stage_ = WashStage::WASH;
            switch (step_index_) {
            case 0:
                execute_step(CycleStep::FILL_MAIN);
                break;
            case 1:
                execute_step(CycleStep::AGITATE_NORMAL);
                break;
            case 2:
                execute_step(CycleStep::DRAIN);
                break;
            default:
                // Wash stage finished! Switch to Rinse & Spin
                in_rinse_subcycle_ = true;
                step_index_ = 0;
                program_ = WashProgram::RINSE_ONLY;
                plan_next_step();
                break;
            }
        }
        break;

    case WashProgram::HEAVY_WASH:
        if (!in_rinse_subcycle_) {
            current_stage_ = WashStage::WASH;
            switch (step_index_) {
            case 0:
                execute_step(CycleStep::FILL_MAIN);
                break;
            case 1:
                execute_step(CycleStep::AGITATE_GENTLE);
                break;
            case 2:
                execute_step(CycleStep::SOAK);
                break;
            case 3:
                execute_step(CycleStep::AGITATE_NORMAL);
                break;
            case 4:
                execute_step(CycleStep::DRAIN);
                break;
            default:
                // Heavy wash stage finished! Switch to Rinse & Spin
                in_rinse_subcycle_ = true;
                step_index_ = 0;
                program_ = WashProgram::RINSE_ONLY;
                plan_next_step();
                break;
            }
        }
        break;
    }
}

} // namespace fsm
