#include "spin_controller.hpp"

namespace controllers {

SpinController::SpinController(
    hal::ITimerHAL& timer_hal,
    hal::IDigitalOutput& drain_pump,
    hal::IReversibleMotor& motor,
    const SpinConfig& config,
    IVibrationMonitor* vibration_monitor)
    : timer_hal_(timer_hal)
    , drain_pump_(drain_pump)
    , motor_(motor)
    , config_(config)
    , vibration_monitor_(vibration_monitor)
{
}

void SpinController::start(domain::WaterLevel level, uint32_t duration_sec)
{
    level_ = level;
    duty_run_duration_ms_ = duration_sec * 1000;

    current_sprint_idx_ = 0;
    total_sprints_ = config_.sprint_count;
    if (config_.sprints && total_sprints_ > 0) {
        current_sprint_on_ms_ = config_.sprints[0].on_ms;
        current_sprint_off_ms_ = config_.sprints[0].off_ms;
    }
    else {
        current_sprint_on_ms_ = 0;
        current_sprint_off_ms_ = 0;
    }

    is_active_ = true;
    is_paused_ = false;
    is_finished_ = false;
    has_error_ = false;
    unbalance_retries_ = 0;
    duty_run_elapsed_before_pause_ms_ = 0;

    if (vibration_monitor_ != nullptr) {
        vibration_monitor_->reset();
    }

    // Always engage drain pump throughout entire spin
    drain_pump_.turn_on();
    motor_.stop();

    sub_phase_ = SpinSubPhase::CLUTCH_ENGAGE;
    resume_target_phase_ = SpinSubPhase::SPRINT_ON;
    phase_start_ms_ = timer_hal_.get_time_ms();
}

void SpinController::update()
{
    if (!is_active_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();

    bool is_spinning =
        (sub_phase_ == SpinSubPhase::SPRINT_ON || sub_phase_ == SpinSubPhase::SPRINT_OFF ||
         sub_phase_ == SpinSubPhase::DUTY_RUN_ON || sub_phase_ == SpinSubPhase::DUTY_RUN_OFF);

    // Out-of-balance safety monitor during active spin
    if (vibration_monitor_ != nullptr && is_spinning) {
        vibration_monitor_->update();
        if (vibration_monitor_->is_critical_unbalance()) {
            motor_.stop();
            if (unbalance_retries_ < config_.max_unbalance_retries) {
                unbalance_retries_++;
                sub_phase_ = SpinSubPhase::RETRY_COASTING;
                phase_start_ms_ = now;
            }
            else {
                has_error_ = true;
                sub_phase_ = SpinSubPhase::STOP_COASTING;
                phase_start_ms_ = now;
            }
            return;
        }
    }

    uint32_t elapsed_in_phase = now - phase_start_ms_;

    switch (sub_phase_) {
    case SpinSubPhase::CLUTCH_ENGAGE:
        if (elapsed_in_phase >= config_.clutch_engage_ms) {
            sub_phase_ = resume_target_phase_;
            phase_start_ms_ = now;
            if (sub_phase_ == SpinSubPhase::DUTY_RUN_ON || sub_phase_ == SpinSubPhase::DUTY_RUN_OFF) {
                duty_run_start_ms_ = now;
            }
            motor_.rotate_clockwise();
        }
        break;

    case SpinSubPhase::SPRINT_ON:
        if (elapsed_in_phase >= current_sprint_on_ms_) {
            motor_.stop();
            sub_phase_ = SpinSubPhase::SPRINT_OFF;
            phase_start_ms_ = now;
        }
        break;

    case SpinSubPhase::SPRINT_OFF:
        if (elapsed_in_phase >= current_sprint_off_ms_) {
            current_sprint_idx_++;
            if (current_sprint_idx_ >= total_sprints_) {
                // All sprints completed -> enter duty run regime
                transition_to_duty_run();
            }
            else {
                current_sprint_on_ms_ = config_.sprints[current_sprint_idx_].on_ms;
                current_sprint_off_ms_ = config_.sprints[current_sprint_idx_].off_ms;
                sub_phase_ = SpinSubPhase::SPRINT_ON;
                phase_start_ms_ = now;
                motor_.rotate_clockwise();
            }
        }
        break;

    case SpinSubPhase::DUTY_RUN_ON:
    case SpinSubPhase::DUTY_RUN_OFF:
    {
        uint32_t total_duty_elapsed = duty_run_elapsed_before_pause_ms_ + (now - duty_run_start_ms_);
        if (total_duty_elapsed >= duty_run_duration_ms_) {
            motor_.stop();
            sub_phase_ = SpinSubPhase::COAST_DOWN;
            phase_start_ms_ = now;
            return;
        }

        if (sub_phase_ == SpinSubPhase::DUTY_RUN_ON) {
            if (elapsed_in_phase >= config_.duty_on_ms) {
                motor_.stop();
                sub_phase_ = SpinSubPhase::DUTY_RUN_OFF;
                phase_start_ms_ = now;
            }
        }
        else {
            if (elapsed_in_phase >= config_.duty_off_ms) {
                motor_.rotate_clockwise();
                sub_phase_ = SpinSubPhase::DUTY_RUN_ON;
                phase_start_ms_ = now;
            }
        }
        break;
    }

    case SpinSubPhase::COAST_DOWN:
        if (elapsed_in_phase >= config_.coast_down_ms) {
            emergency_stop();
            is_finished_ = true;
        }
        break;

    case SpinSubPhase::PAUSE_COASTING:
        if (elapsed_in_phase >= config_.coast_down_ms) {
            drain_pump_.turn_off();
            sub_phase_ = SpinSubPhase::PAUSED;
            is_paused_ = true;
        }
        break;

    case SpinSubPhase::STOP_COASTING:
        if (elapsed_in_phase >= config_.coast_down_ms) {
            emergency_stop();
        }
        break;

    case SpinSubPhase::RETRY_COASTING:
        if (elapsed_in_phase >= config_.coast_down_ms) {
            // Drum reached full standstill (0 RPM). Reset monitor and restart progressive ramp from Sprint 1
            if (vibration_monitor_ != nullptr) {
                vibration_monitor_->reset();
            }
            current_sprint_idx_ = 0;
            if (config_.sprints && total_sprints_ > 0) {
                current_sprint_on_ms_ = config_.sprints[0].on_ms;
                current_sprint_off_ms_ = config_.sprints[0].off_ms;
            }
            sub_phase_ = SpinSubPhase::CLUTCH_ENGAGE;
            resume_target_phase_ = SpinSubPhase::SPRINT_ON;
            phase_start_ms_ = now;
        }
        break;

    case SpinSubPhase::PAUSED:
    case SpinSubPhase::IDLE:
    default:
        break;
    }
}

void SpinController::transition_to_duty_run()
{
    sub_phase_ = SpinSubPhase::DUTY_RUN_ON;
    resume_target_phase_ = SpinSubPhase::DUTY_RUN_ON;
    uint32_t now = timer_hal_.get_time_ms();
    phase_start_ms_ = now;
    duty_run_start_ms_ = now;
    motor_.rotate_clockwise();
}

void SpinController::pause()
{
    if (!is_active_ || is_paused_ || sub_phase_ == SpinSubPhase::PAUSE_COASTING) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();

    if (sub_phase_ == SpinSubPhase::CLUTCH_ENGAGE) {
        // Motor hasn't rotated yet, pump can turn off safely
        drain_pump_.turn_off();
        sub_phase_ = SpinSubPhase::PAUSED;
        is_paused_ = true;
        return;
    }

    if (sub_phase_ == SpinSubPhase::DUTY_RUN_ON || sub_phase_ == SpinSubPhase::DUTY_RUN_OFF) {
        duty_run_elapsed_before_pause_ms_ += (now - duty_run_start_ms_);
        resume_target_phase_ = SpinSubPhase::DUTY_RUN_ON;
    }

    // Cut motor immediately, but keep pump on for coast-down to protect transmission
    motor_.stop();
    sub_phase_ = SpinSubPhase::PAUSE_COASTING;
    phase_start_ms_ = now;
}

void SpinController::resume()
{
    if (!is_active_ || (!is_paused_ && sub_phase_ != SpinSubPhase::PAUSE_COASTING)) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    drain_pump_.turn_on();
    is_paused_ = false;

    // Re-engage clutch before spinning motor again
    sub_phase_ = SpinSubPhase::CLUTCH_ENGAGE;
    phase_start_ms_ = now;
}

void SpinController::stop()
{
    if (!is_active_) {
        return;
    }

    if (sub_phase_ == SpinSubPhase::CLUTCH_ENGAGE || sub_phase_ == SpinSubPhase::PAUSED) {
        emergency_stop();
        return;
    }

    // Soft stop with coast-down protection
    motor_.stop();
    sub_phase_ = SpinSubPhase::STOP_COASTING;
    phase_start_ms_ = timer_hal_.get_time_ms();
}

void SpinController::emergency_stop()
{
    motor_.stop();
    drain_pump_.turn_off();
    sub_phase_ = SpinSubPhase::IDLE;
    is_active_ = false;
    is_paused_ = false;
}

} // namespace controllers
