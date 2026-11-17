#pragma once

#include <stdint.h>

namespace controllers {

/**
 * @enum DampingAction
 * @brief Discrete actions dictated by the adaptive vibration damping policy.
 */
enum class DampingAction : uint8_t
{
    NONE = 0, //!< No intervention; continue current state
    PAUSE,    //!< De-energize motor immediately to damp vibration
    RESUME,   //!< Re-energize motor after vibration cleared and min pause elapsed
    ESCALATE  //!< Escalate to retry or stop due to critical trip, timeout, or sensor failure
};

/**
 * @struct DampingInput
 * @brief Snapshot of vibration, sensor health, and timing inputs passed to the policy.
 */
struct DampingInput
{
    bool sensor_ok{true};               //!< Accelerometer communication health
    bool warning{false};                //!< Perceptible unbalance flag (active warning)
    bool critical{false};               //!< Critical trip flag
    uint32_t now_ms{0};                 //!< Current timestamp in milliseconds
    uint32_t damping_elapsed_ms{0};     //!< Time elapsed in active damping phase
    uint32_t sensor_bad_duration_ms{0}; //!< Continuous duration of sensor communication failure
};

/**
 * @struct DampingConfig
 * @brief Configurable timing thresholds for damping evaluation.
 */
struct DampingConfig
{
    uint32_t min_off_ms{2500};        //!< Minimum resting pause before re-energizing motor
    uint32_t max_off_ms{8000};        //!< Maximum allowed resting pause before escalating
    uint32_t sensor_timeout_ms{1000}; //!< Continuous sensor failure before escalating
};

/**
 * @class VibrationDampingPolicy
 * @brief Pure deterministic decision policy governing vibration pause, resume, and escalation.
 *
 * Implements pure logic with zero HAL dependencies, suitable for host unit testing.
 */
class VibrationDampingPolicy
{
public:
    explicit VibrationDampingPolicy(const DampingConfig& config = DampingConfig{});

    /**
     * @brief Evaluate current machine and vibration status to determine required action.
     * @param input Current sensor health, envelope warnings, and elapsed times.
     * @param motor_energized Whether motor is currently commanded ON.
     * @param damping_active Whether controller is currently in VIBE_DAMPING state.
     * @return DampingAction to execute.
     */
    DampingAction evaluate(const DampingInput& input, bool motor_energized, bool damping_active) const;

    /**
     * @brief Check whether conditions permit resuming motor acceleration.
     * @param input Current sensor health and warning state.
     * @param damping_elapsed_ms Time elapsed in damping phase.
     * @return true if safe to re-energize motor.
     */
    bool can_resume(const DampingInput& input, uint32_t damping_elapsed_ms) const;

    /**
     * @brief Get active damping configuration (after any clamping).
     * @return Reference to DampingConfig.
     */
    const DampingConfig& get_config() const { return config_; }

    /**
     * @brief Check whether runtime configuration passed invariant checks.
     * @return true if valid, false if clamped to safe defaults.
     */
    bool is_config_valid() const { return is_config_valid_; }

private:
    DampingConfig config_;
    bool is_config_valid_{true};
};

} // namespace controllers
