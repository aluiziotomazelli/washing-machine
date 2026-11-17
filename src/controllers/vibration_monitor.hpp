#pragma once

#include <stdint.h>
#include "interfaces/i_vibration_monitor.hpp"
#include "../hal/interfaces/i_accelerometer.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace controllers {

/**
 * @struct VibrationConfig
 * @brief Calibration parameters for vibration detection based on empirical load tests.
 */
struct VibrationConfig
{
    uint16_t sample_period_ms{20};             //!< Acquisition period (20 ms = 50 Hz).
    uint8_t window_samples{10};                //!< Samples per peak-to-peak window (10 = 200 ms).
    uint16_t motion_threshold{400};            //!< Minimum envelope to report physical movement.
    uint16_t accel_warning_threshold{5500};    //!< Envelope that flags unbalance during ACCEL (damping).
    uint16_t trip_threshold{11000};            //!< Sustained envelope that triggers a trip.
    uint16_t shock_threshold{14000};           //!< Single-sample envelope that triggers an immediate trip.
    uint16_t sustained_trip_duration_ms{1000}; //!< Time above trip_threshold before latching a trip.
    uint16_t accel_warning_clear{4000};        //!< Hysteresis clear during ACCEL (proportional to warning).
    uint16_t warning_debounce_ms{0};           //!< Debounce before latch (0 = first 200 ms window).
};

/**
 * @class VibrationMonitor
 * @brief Autonomous vibration analysis engine computing 3D peak-to-peak envelopes.
 */
class VibrationMonitor : public IVibrationMonitor
{
public:
    /**
     * @brief Construct a vibration monitor bound to an accelerometer and timer.
     * @param accelerometer Accelerometer source sampled by the monitor.
     * @param timer_hal Timer abstraction used for sampling and trip timing.
     * @param config Calibration thresholds and sampling window.
     */
    VibrationMonitor(
        hal::IAccelerometer& accelerometer,
        hal::ITimerHAL& timer_hal,
        const VibrationConfig& config = VibrationConfig{});

    /** @copydoc IVibrationMonitor::init */
    void init() override;
    /** @copydoc IVibrationMonitor::update */
    void update() override;

    /** @copydoc IVibrationMonitor::get_vibration */
    uint16_t get_vibration() const override { return current_vib_; }
    /** @copydoc IVibrationMonitor::is_in_motion */
    bool is_in_motion() const override { return is_in_motion_; }
    /** @copydoc IVibrationMonitor::is_warning */
    bool is_warning() const override { return is_warning_; }
    /** @copydoc IVibrationMonitor::is_critical_unbalance */
    bool is_critical_unbalance() const override { return is_tripped_; }
    /** @copydoc IVibrationMonitor::is_sensor_ok */
    bool is_sensor_ok() const override { return is_sensor_ok_; }

    /** @copydoc IVibrationMonitor::reset */
    void reset() override;

    /** @copydoc IVibrationMonitor::set_accel_damping_mode */
    void set_accel_damping_mode(bool enabled) override;

    /**
     * @brief Enable or disable sample acquisition (disabled monitor keeps last state).
     * @param enabled true to resume sampling.
     */
    void set_enabled(bool enabled) { is_enabled_ = enabled; }

    /**
     * @brief Check whether sampling is currently enabled.
     * @return true when the monitor is sampling.
     */
    bool is_enabled() const { return is_enabled_; }

    /**
     * @brief Check whether runtime configuration passed invariant checks.
     * @return true if valid, false if clamped to safe defaults.
     */
    bool is_config_valid() const { return is_config_valid_; }

    /**
     * @brief Get active configuration (after any clamping).
     * @return Reference to VibrationConfig.
     */
    const VibrationConfig& get_config() const { return config_; }

private:
    void process_sample(const hal::Vector3& sample, uint32_t now);
    void reset_window_extents();

    hal::IAccelerometer& accel_;
    hal::ITimerHAL& timer_hal_;
    VibrationConfig config_;

    bool is_config_valid_{true};
    bool is_enabled_{true};
    bool accel_damping_mode_{false};
    uint32_t last_sample_ms_{0};

    // Current window min/max tracking
    int16_t min_x_{INT16_MAX};
    int16_t max_x_{INT16_MIN};
    int16_t min_y_{INT16_MAX};
    int16_t max_y_{INT16_MIN};
    int16_t min_z_{INT16_MAX};
    int16_t max_z_{INT16_MIN};
    uint8_t sample_count_{0};

    // Evaluated state
    uint16_t current_vib_{0};
    bool is_in_motion_{false};
    bool is_warning_{false};
    bool is_tripped_{false};
    bool is_sensor_ok_{false};
    uint32_t last_read_success_ms_{0};

    // Temporal trip tracking
    uint32_t trip_start_ms_{0};
    uint32_t warning_since_ms_{0};
};

} // namespace controllers
