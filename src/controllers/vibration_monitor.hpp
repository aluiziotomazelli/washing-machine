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
struct VibrationConfig {
    uint16_t sample_period_ms{20};            // 50 Hz acquisition rate
    uint8_t window_samples{10};               // 10 samples = 200 ms peak-to-peak window
    uint16_t motion_threshold{400};           // > 400 LSB: mechanical movement detected
    uint16_t warning_threshold{8500};         // 8500 LSB: visible unbalance warning
    uint16_t trip_threshold{11000};           // 11000 LSB: critical sustained unbalance limit
    uint16_t shock_threshold{14000};          // 14000 LSB: single shock trip limit (foot-slip zone)
    uint16_t sustained_trip_duration_ms{1000}; // 1000 ms sustained limit for trip_threshold
};

/**
 * @class VibrationMonitor
 * @brief Autonomous vibration analysis engine computing 3D peak-to-peak envelopes.
 */
class VibrationMonitor : public IVibrationMonitor {
public:
    VibrationMonitor(
        hal::IAccelerometer& accelerometer,
        hal::ITimerHAL& timer_hal,
        const VibrationConfig& config = VibrationConfig{}
    );

    void init() override;
    void update() override;

    uint16_t get_vibration() const override { return current_vib_; }
    const hal::Vector3& get_last_sample() const override { return last_sample_; }
    bool is_in_motion() const override { return is_in_motion_; }
    bool is_warning() const override { return is_warning_; }
    bool is_critical_unbalance() const override { return is_tripped_; }
    bool is_sensor_ok() const override { return is_sensor_ok_; }

    void reset() override;

    void set_enabled(bool enabled) { is_enabled_ = enabled; }
    bool is_enabled() const { return is_enabled_; }

private:
    void process_sample(const hal::Vector3& sample, uint32_t now);
    void reset_window_extents();

    hal::IAccelerometer& accel_;
    hal::ITimerHAL& timer_hal_;
    VibrationConfig config_;

    bool is_enabled_{true};
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
    hal::Vector3 last_sample_{};
    uint16_t current_vib_{0};
    bool is_in_motion_{false};
    bool is_warning_{false};
    bool is_tripped_{false};
    bool is_sensor_ok_{false};
    uint32_t last_read_success_ms_{0};

    // Temporal trip tracking
    uint32_t trip_start_ms_{0};
};

} // namespace controllers
