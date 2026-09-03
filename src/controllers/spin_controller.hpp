#pragma once

#include <stdint.h>
#include "../domain/wash_types.hpp"
#include "../hal/interfaces/i_digital_output.hpp"
#include "../hal/interfaces/i_reversible_motor.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace controllers {

/**
 * @enum SpinSubPhase
 * @brief Internal non-blocking states of the centrifugal extraction cycle.
 */
enum class SpinSubPhase : uint8_t
{
    IDLE = 0,
    CLUTCH_ENGAGE,  // Wait for mechanical clutch/brake to engage
    SPRINT_ON,      // Motor sprint pulse
    SPRINT_OFF,     // Resting pause between sprints
    DUTY_RUN_ON,    // Continuous extraction: 4s motor pulse
    DUTY_RUN_OFF,   // Continuous extraction: 4s inertia coast
    COAST_DOWN,     // Normal completion drum spin-down before pump off
    PAUSE_COASTING, // Controlled deceleration during pause with pump on
    PAUSED,         // Drum fully stopped and pump off
    STOP_COASTING   // Controlled deceleration during stop with pump on
};

/**
 * @struct SprintStep
 * @brief Represents a single sprint phase with tailored motor run and pump pause times.
 */
struct SprintStep
{
    uint16_t on_ms;
    uint16_t off_ms;
};

// Default progressive sprint profile tuned for top-load suspension dynamics
inline constexpr SprintStep k_default_sprints[] = {
    { 4000, 3500 }, // S1: Initial pull & clothing distribution without dying
    { 5000, 3500 }, // S2: Water expulsion with short pause to prevent coasting resonance
    { 6000, 4000 }, // S3: Speed ramp & suspension stabilization
    { 7000, 3000 }  // S4: Cutoff before resonance peak & high-speed handover to cruise
};

/**
 * @struct SpinConfig
 * @brief Configuration timings for spin cycle phases (in milliseconds).
 */
struct SpinConfig
{
    uint32_t clutch_engage_ms{5000}; // 5s clutch engagement delay
    uint32_t duty_on_ms{4000};       // 4s motor pulse
    uint32_t duty_off_ms{4000};      // 4s inertia coast
    uint32_t coast_down_ms{10000};   // 10s drum coast-down before pump off
    const SprintStep* sprints{k_default_sprints};
    uint8_t sprint_count{sizeof(k_default_sprints) / sizeof(k_default_sprints[0])};
};

/**
 * @class SpinController
 * @brief Manages centrifugal extraction sequence: clutch engage -> dynamic sprints -> 4s/4s inertia duty cycle -> coast
 * down.
 *
 * Features mechanical transmission protection: routes pause() and stop() through non-blocking coast-down,
 * keeping the pump/actuator engaged until the drum slows down to avoid violent mechanical braking.
 */
class SpinController
{
public:
    SpinController(
        hal::ITimerHAL& timer_hal,
        hal::IDigitalOutput& drain_pump,
        hal::IReversibleMotor& motor,
        const SpinConfig& config = SpinConfig{});

    /**
     * @brief Start centrifugal spin cycle.
     * @param level Water level selected (determines sprint inertia profile).
     * @param duration_sec Total duty-run spin duration in seconds (default 4 min).
     */
    void start(domain::WaterLevel level, uint32_t duration_sec = 240);

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

    bool is_active() const { return is_active_; }
    bool is_paused() const { return is_paused_; }
    bool is_finished() const { return is_finished_; }
    SpinSubPhase get_sub_phase() const { return sub_phase_; }

private:
    void transition_to_duty_run();

    hal::ITimerHAL& timer_hal_;
    hal::IDigitalOutput& drain_pump_;
    hal::IReversibleMotor& motor_;
    SpinConfig config_;

    domain::WaterLevel level_{domain::WaterLevel::LOW_LEVEL};
    uint32_t duty_run_duration_ms_{0};

    SpinSubPhase sub_phase_{SpinSubPhase::IDLE};
    SpinSubPhase resume_target_phase_{SpinSubPhase::DUTY_RUN_ON};
    uint32_t phase_start_ms_{0};

    // Sprint state
    uint8_t current_sprint_idx_{0};
    uint8_t total_sprints_{4};
    uint16_t current_sprint_on_ms_{4000};
    uint16_t current_sprint_off_ms_{3500};

    // Duty Run tracking
    uint32_t duty_run_start_ms_{0};
    uint32_t duty_run_elapsed_before_pause_ms_{0};

    bool is_active_{false};
    bool is_paused_{false};
    bool is_finished_{false};
};

} // namespace controllers
