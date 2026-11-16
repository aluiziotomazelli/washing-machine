#include "vibration_damping_policy.hpp"

namespace controllers {

VibrationDampingPolicy::VibrationDampingPolicy(const DampingConfig& config)
    : config_(config)
{
    const DampingConfig defaults{};

    if (config_.sensor_timeout_ms == 0) {
        config_.sensor_timeout_ms = defaults.sensor_timeout_ms;
        is_config_valid_ = false;
    }

    if (config_.min_off_ms == 0 || config_.min_off_ms >= config_.max_off_ms) {
        config_.min_off_ms = defaults.min_off_ms;
        config_.max_off_ms = defaults.max_off_ms;
        is_config_valid_ = false;
    }
}

DampingAction
VibrationDampingPolicy::evaluate(const DampingInput& input, bool motor_energized, bool damping_active) const
{
    // Priority 1: Critical unbalance trips take absolute precedence
    if (input.critical) {
        return DampingAction::ESCALATE;
    }

    // Priority 2: Unhealthy sensor exceeding timeout
    if (!input.sensor_ok && (input.sensor_bad_duration_ms >= config_.sensor_timeout_ms)) {
        return DampingAction::ESCALATE;
    }

    // Priority 3: Warning detected while motor is running -> cut power immediately
    if (input.warning && motor_energized) {
        return DampingAction::PAUSE;
    }

    // Priority 4: In active damping, safe to resume if warning cleared and min_off satisfied
    if (damping_active && can_resume(input, input.damping_elapsed_ms)) {
        return DampingAction::RESUME;
    }

    // Priority 5: In active damping, unbalance persisted beyond maximum off window -> escalate
    if (damping_active && (input.damping_elapsed_ms >= config_.max_off_ms)) {
        return DampingAction::ESCALATE;
    }

    // Priority 6: Default normal state
    return DampingAction::NONE;
}

bool VibrationDampingPolicy::can_resume(const DampingInput& input, uint32_t damping_elapsed_ms) const
{
    return input.sensor_ok && !input.warning && !input.critical && (damping_elapsed_ms >= config_.min_off_ms);
}

} // namespace controllers
