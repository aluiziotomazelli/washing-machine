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
    , damping_policy_(
          DampingConfig{config_.damping_min_off_ms, config_.damping_max_off_ms, config_.damping_sensor_timeout_ms})
    , vibration_monitor_(vibration_monitor)
{}

void SpinController::start(domain::WaterLevel level, uint32_t duration_sec)
{
    level_ = level;
    duty_run_duration_ms_ = duration_sec * 1000;

    is_active_ = true;
    is_paused_ = false;
    is_finished_ = false;
    duty_run_elapsed_before_pause_ms_ = 0;
    duty_run_start_ms_ = 0;
    damping_phase_elapsed_ms_ = 0;
    damping_start_ms_ = 0;
    sensor_bad_since_ms_ = 0;
    sensor_bad_ = false;
    accel_timeline_.reset();
    reset_error();

    drain_pump_.turn_on();
    motor_.stop();

    sub_phase_ = SpinSubPhase::CLUTCH_ENGAGE;
    resume_target_phase_ = SpinSubPhase::ACCEL;
    damping_return_phase_ = SpinSubPhase::ACCEL;
    phase_start_ms_ = timer_hal_.get_time_ms();
}

void SpinController::update_sensor_bad_tracking(uint32_t now)
{
    if (vibration_monitor_ == nullptr) {
        sensor_bad_ = false;
        sensor_bad_since_ms_ = 0;
        return;
    }

    bool ok = vibration_monitor_->is_sensor_ok();
    if (ok) {
        sensor_bad_ = false;
        sensor_bad_since_ms_ = 0;
        return;
    }

    if (!sensor_bad_) {
        sensor_bad_ = true;
        sensor_bad_since_ms_ = now;
    }
}

uint32_t SpinController::sensor_bad_duration_ms(uint32_t now) const
{
    if (!sensor_bad_) {
        return 0;
    }
    return (now >= sensor_bad_since_ms_) ? (now - sensor_bad_since_ms_) : 0;
}

DampingInput SpinController::make_damping_input(uint32_t now) const
{
    DampingInput input{};
    if (vibration_monitor_ != nullptr) {
        input.sensor_ok = vibration_monitor_->is_sensor_ok();
        input.warning = vibration_monitor_->is_warning();
        input.critical = vibration_monitor_->is_critical_unbalance();
    }
    input.now_ms = now;
    input.damping_elapsed_ms = (now >= damping_start_ms_) ? (now - damping_start_ms_) : 0;
    input.sensor_bad_duration_ms = sensor_bad_duration_ms(now);
    return input;
}

void SpinController::enter_retry_or_stop(uint32_t now)
{
    if (sub_phase_ == SpinSubPhase::ACCEL) {
        accel_timeline_.on_motor_off(now);
    }
    if (sub_phase_ == SpinSubPhase::DUTY_RUN_ON) {
        duty_run_elapsed_before_pause_ms_ += (now - duty_run_start_ms_);
        duty_run_start_ms_ = now;
    }

    motor_.stop();

    if (unbalance_retries_ < config_.max_unbalance_retries) {
        unbalance_retries_++;
        sub_phase_ = SpinSubPhase::RETRY_COASTING;
    }
    else {
        sub_phase_ = SpinSubPhase::STOP_COASTING;
    }
    phase_start_ms_ = now;
}

void SpinController::enter_vibe_damping(uint32_t now)
{
    if (sub_phase_ != SpinSubPhase::ACCEL && sub_phase_ != SpinSubPhase::DUTY_RUN_ON) {
        return;
    }

    damping_return_phase_ = sub_phase_;
    damping_phase_elapsed_ms_ = now - phase_start_ms_;
    damping_start_ms_ = now;

    if (sub_phase_ == SpinSubPhase::DUTY_RUN_ON) {
        duty_run_elapsed_before_pause_ms_ += (now - duty_run_start_ms_);
        duty_run_start_ms_ = now;
    }
    if (sub_phase_ == SpinSubPhase::ACCEL) {
        accel_timeline_.on_motor_off(now);
    }

    motor_.stop();
    sub_phase_ = SpinSubPhase::VIBE_DAMPING;
    phase_start_ms_ = now;
}

void SpinController::resume_from_vibe_damping(uint32_t now)
{
    sub_phase_ = damping_return_phase_;
    if (sub_phase_ == SpinSubPhase::ACCEL) {
        phase_start_ms_ = now;
        motor_.rotate_clockwise();
        accel_timeline_.on_motor_on(now);
        return;
    }

    if (sub_phase_ == SpinSubPhase::DUTY_RUN_ON) {
        duty_run_start_ms_ = now;
        phase_start_ms_ = now - damping_phase_elapsed_ms_;
        motor_.rotate_clockwise();
        return;
    }

    sub_phase_ = SpinSubPhase::ACCEL;
    resume_target_phase_ = SpinSubPhase::ACCEL;
    phase_start_ms_ = now;
}

void SpinController::handle_vibe_damping(uint32_t now)
{
    DampingInput input = make_damping_input(now);
    DampingAction action = damping_policy_.evaluate(input, false, true);
    if (action == DampingAction::ESCALATE) {
        enter_retry_or_stop(now);
        return;
    }
    if (action == DampingAction::RESUME || damping_policy_.can_resume(input, input.damping_elapsed_ms)) {
        resume_from_vibe_damping(now);
    }
}

void SpinController::update()
{
    if (!is_active_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();

    bool monitor_active =
        (sub_phase_ == SpinSubPhase::ACCEL || sub_phase_ == SpinSubPhase::VIBE_DAMPING ||
         sub_phase_ == SpinSubPhase::DUTY_RUN_ON || sub_phase_ == SpinSubPhase::DUTY_RUN_OFF);

    if (vibration_monitor_ != nullptr && monitor_active) {
        vibration_monitor_->update();
        update_sensor_bad_tracking(now);

        if (vibration_monitor_->is_critical_unbalance()) {
            enter_retry_or_stop(now);
            return;
        }

        bool motor_energized = (sub_phase_ == SpinSubPhase::ACCEL || sub_phase_ == SpinSubPhase::DUTY_RUN_ON);
        if (motor_energized && vibration_monitor_->is_warning()) {
            enter_vibe_damping(now);
            return;
        }

        if (sub_phase_ == SpinSubPhase::VIBE_DAMPING) {
            handle_vibe_damping(now);
            if (sub_phase_ == SpinSubPhase::VIBE_DAMPING || sub_phase_ == SpinSubPhase::RETRY_COASTING ||
                sub_phase_ == SpinSubPhase::STOP_COASTING) {
                return;
            }
        }
    }

    uint32_t elapsed_in_phase = now - phase_start_ms_;

    switch (sub_phase_) {
    case SpinSubPhase::CLUTCH_ENGAGE:
        if (elapsed_in_phase >= config_.clutch_engage_ms) {
            sub_phase_ = resume_target_phase_;
            phase_start_ms_ = now;
            if (sub_phase_ == SpinSubPhase::ACCEL) {
                accel_timeline_.start(now, config_.accel_motor_on_ms, config_.accel_max_wall_ms);
                motor_.rotate_clockwise();
                accel_timeline_.on_motor_on(now);
            }
            else if (sub_phase_ == SpinSubPhase::DUTY_RUN_ON || sub_phase_ == SpinSubPhase::DUTY_RUN_OFF) {
                duty_run_start_ms_ = now;
                motor_.rotate_clockwise();
            }
        }
        break;

    case SpinSubPhase::ACCEL:
        accel_timeline_.on_motor_on(now);
        if (accel_timeline_.is_complete(now)) {
            accel_timeline_.on_motor_off(now);
            transition_to_duty_run();
        }
        break;

    case SpinSubPhase::VIBE_DAMPING:
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
            bool was_unbalanced = (unbalance_retries_ >= config_.max_unbalance_retries);
            emergency_stop();
            if (was_unbalanced) {
                has_error_ = true;
            }
            else {
                is_finished_ = true;
            }
        }
        break;

    case SpinSubPhase::RETRY_COASTING:
        if (elapsed_in_phase >= config_.coast_down_ms) {
            if (vibration_monitor_ != nullptr) {
                vibration_monitor_->reset();
            }
            sensor_bad_ = false;
            sensor_bad_since_ms_ = 0;
            accel_timeline_.reset();
            sub_phase_ = SpinSubPhase::CLUTCH_ENGAGE;
            resume_target_phase_ = SpinSubPhase::ACCEL;
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
        drain_pump_.turn_off();
        sub_phase_ = SpinSubPhase::PAUSED;
        is_paused_ = true;
        return;
    }

    if (sub_phase_ == SpinSubPhase::DUTY_RUN_ON || sub_phase_ == SpinSubPhase::DUTY_RUN_OFF) {
        duty_run_elapsed_before_pause_ms_ += (now - duty_run_start_ms_);
        duty_run_start_ms_ = now;
    }

    motor_.stop();
    if (sub_phase_ == SpinSubPhase::ACCEL) {
        accel_timeline_.on_motor_off(now);
    }
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

    if (vibration_monitor_ != nullptr) {
        vibration_monitor_->reset();
    }
    sensor_bad_ = false;
    sensor_bad_since_ms_ = 0;

    accel_timeline_.reset();
    resume_target_phase_ = SpinSubPhase::ACCEL;

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

    if (sub_phase_ == SpinSubPhase::STOP_COASTING) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    motor_.stop();
    if (sub_phase_ == SpinSubPhase::ACCEL) {
        accel_timeline_.on_motor_off(now);
    }
    if (sub_phase_ == SpinSubPhase::DUTY_RUN_ON) {
        duty_run_elapsed_before_pause_ms_ += (now - duty_run_start_ms_);
        duty_run_start_ms_ = now;
    }
    sub_phase_ = SpinSubPhase::STOP_COASTING;
    phase_start_ms_ = now;
}

void SpinController::emergency_stop()
{
    motor_.stop();
    drain_pump_.turn_off();
    accel_timeline_.reset();
    sensor_bad_ = false;
    sensor_bad_since_ms_ = 0;
    sub_phase_ = SpinSubPhase::IDLE;
    is_active_ = false;
    is_paused_ = false;
}

void SpinController::reset_error()
{
    has_error_ = false;
    unbalance_retries_ = 0;
    sensor_bad_ = false;
    sensor_bad_since_ms_ = 0;
    if (vibration_monitor_ != nullptr) {
        vibration_monitor_->reset();
    }
}

} // namespace controllers
