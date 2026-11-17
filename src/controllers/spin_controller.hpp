#pragma once

#include <stdint.h>
#include "../domain/wash_types.hpp"
#include "../hal/interfaces/i_digital_output.hpp"
#include "../hal/interfaces/i_reversible_motor.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"
#include "accel_timeline.hpp"
#include "interfaces/i_vibration_monitor.hpp"
#include "vibration_damping_policy.hpp"

namespace controllers {

/**
 * @enum SpinSubPhase
 * @brief Internal non-blocking states of the centrifugal extraction cycle.
 */
enum class SpinSubPhase : uint8_t
{
    IDLE = 0,
    CLUTCH_ENGAGE,  // Wait for mechanical clutch/brake to engage
    ACCEL,          // Sensor-driven continuous acceleration (motor-on time accumulated)
    VIBE_DAMPING,   // Adaptive pause while vibration warning is active
    DUTY_RUN_ON,    // Continuous extraction: motor pulse
    DUTY_RUN_OFF,   // Continuous extraction: inertia coast
    COAST_DOWN,     // Normal completion drum spin-down before pump off
    PAUSE_COASTING, // Controlled deceleration during pause with pump on
    PAUSED,         // Drum fully stopped and pump off
    STOP_COASTING,  // Controlled deceleration during stop with pump on
    RETRY_COASTING  // Controlled deceleration after unbalance trip before re-attempting spin
};

/**
 * @struct SpinConfig
 * @brief Configuration timings for spin cycle phases (in milliseconds).
 */
struct SpinConfig
{
    uint32_t clutch_engage_ms{5000};          //!< Clutch engagement delay in milliseconds.
    uint32_t duty_on_ms{4000};                //!< Continuous extraction motor pulse duration.
    uint32_t duty_off_ms{4000};               //!< Inertia coast duration between extraction pulses.
    uint32_t coast_down_ms{18000};            //!< Drum coast-down time before releasing the pump/brake.
    uint32_t accel_motor_on_ms{8000};         //!< Accumulated motor-on time target for ACCEL (8 s).
    uint32_t accel_max_wall_ms{40000};        //!< Maximum wall-clock cap for ACCEL phase.
    uint32_t damping_min_off_ms{800};         //!< Minimum motor-off time before damping can resume.
    uint32_t damping_max_off_ms{8000};        //!< Maximum damping duration before escalation.
    uint32_t damping_sensor_timeout_ms{1000}; //!< Sensor-failure duration before escalation.
    uint8_t max_unbalance_retries{2};         //!< Dry redistribution retries before latching an error.
};

/**
     * @class SpinController
     * @brief Manages centrifugal extraction sequence: clutch engage -> ACCEL -> 4s/4s inertia duty cycle -> coast down.
     *
     * Features mechanical transmission protection: routes pause() and stop() through non-blocking coast-down,
     * keeping the pump/actuator engaged until the drum slows down to avoid violent mechanical braking.
     * ACCEL is sensor-driven with adaptive vibration damping.
     */
class SpinController
{
public:
    /**
     * @brief Construct a spin controller bound to timer, pump, motor and optional monitor.
     * @param timer_hal Timer abstraction used for phase timing.
     * @param drain_pump Drain pump / brake clutch output kept engaged during coast-down.
     * @param motor Reversible motor used for centrifugal extraction.
     * @param config Acceleration, damping and phase timings.
     * @param vibration_monitor Optional unbalance monitor; nullptr disables the guard.
     */
    SpinController(
        hal::ITimerHAL& timer_hal,
        hal::IDigitalOutput& drain_pump,
        hal::IReversibleMotor& motor,
        const SpinConfig& config = SpinConfig{},
        IVibrationMonitor* vibration_monitor = nullptr);

    /**
     * @brief Start centrifugal spin cycle.
     * @param level Water level selected.
     * @param duration_sec Total duty-run spin duration in seconds (default 4 min).
     */
    void start(domain::WaterLevel level, uint32_t duration_sec = 240);

    /**
     * @brief Periodic non-blocking update (sub-phase progression and unbalance checks).
     */
    void update();

    /**
     * @brief Soft pause: cuts motor immediately but keeps pump energized for coast-down to protect transmission.
     */
    void pause();

    /**
     * @brief Resume spinning: re-engages clutch and continues the cycle.
     */
    void resume();

    /**
     * @brief Soft stop: cuts motor immediately and coasts down before releasing pump/brake.
     */
    void stop();

    /**
     * @brief Immediate emergency cut without coast-down.
     */
    void emergency_stop();

    /**
     * @brief Clear latched error and reset unbalance retry counter.
     */
    void reset_error();

    /**
     * @brief Check whether the spin cycle is running.
     * @return true while active.
     */
    bool is_active() const { return is_active_; }

    /**
     * @brief Check whether the spin cycle is suspended.
     * @return true while paused.
     */
    bool is_paused() const { return is_paused_; }

    /**
     * @brief Check whether the spin cycle completed.
     * @return true when finished.
     */
    bool is_finished() const { return is_finished_; }

    /**
     * @brief Check whether an unbalance error is latched.
     * @return true when the cycle stopped due to unbalance.
     */
    bool has_error() const { return has_error_; }

    /**
     * @brief Get the number of unbalance redistribution retries already used.
     * @return Retry count.
     */
    uint8_t get_unbalance_retries() const { return unbalance_retries_; }

    /**
     * @brief Get the current internal sub-phase.
     * @return Active SpinSubPhase.
     */
    SpinSubPhase get_sub_phase() const { return sub_phase_; }

    /**
     * @brief Check whether runtime configuration passed invariant checks.
     * @return true if valid, false if clamped to safe defaults.
     */
    bool is_config_valid() const { return is_config_valid_; }

    /**
     * @brief Get active configuration (after any clamping).
     * @return Reference to SpinConfig.
     */
    const SpinConfig& get_config() const { return config_; }

private:
    void transition_to_duty_run();
    void enter_vibe_damping(uint32_t now);
    void resume_from_vibe_damping(uint32_t now);
    void enter_retry_or_stop(uint32_t now);
    void handle_vibe_damping(uint32_t now);
    DampingInput make_damping_input(uint32_t now) const;
    void update_sensor_bad_tracking(uint32_t now);
    uint32_t sensor_bad_duration_ms(uint32_t now) const;
    void sync_accel_damping_mode();

    hal::ITimerHAL& timer_hal_;
    hal::IDigitalOutput& drain_pump_;
    hal::IReversibleMotor& motor_;
    SpinConfig config_;
    VibrationDampingPolicy damping_policy_;
    bool is_config_valid_{true};

    domain::WaterLevel level_{domain::WaterLevel::LOW_LEVEL};
    uint32_t duty_run_duration_ms_{0};

    SpinSubPhase sub_phase_{SpinSubPhase::IDLE};
    SpinSubPhase resume_target_phase_{SpinSubPhase::ACCEL};
    SpinSubPhase damping_return_phase_{SpinSubPhase::ACCEL};
    uint32_t phase_start_ms_{0};
    uint32_t damping_phase_elapsed_ms_{0};
    uint32_t damping_start_ms_{0};
    uint32_t sensor_bad_since_ms_{0};
    AccelTimeline accel_timeline_;

    // Duty Run tracking
    uint32_t duty_run_start_ms_{0};
    uint32_t duty_run_elapsed_before_pause_ms_{0};

    bool is_active_{false};
    bool is_paused_{false};
    bool is_finished_{false};
    bool has_error_{false};
    bool sensor_bad_{false};
    uint8_t unbalance_retries_{0};
    IVibrationMonitor* vibration_monitor_{nullptr};
};

} // namespace controllers
