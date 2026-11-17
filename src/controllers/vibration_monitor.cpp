#include "vibration_monitor.hpp"

namespace controllers {

VibrationMonitor::VibrationMonitor(
    hal::IAccelerometer& accelerometer,
    hal::ITimerHAL& timer_hal,
    const VibrationConfig& config)
    : accel_(accelerometer)
    , timer_hal_(timer_hal)
    , config_(config)
{
    // Runtime configuration invariant validation
    bool valid = (config_.accel_warning_clear < config_.accel_warning_threshold) &&
                 (config_.accel_warning_threshold < config_.trip_threshold) &&
                 (config_.trip_threshold <= config_.shock_threshold) &&
                 (config_.motion_threshold < config_.accel_warning_threshold) &&
                 (config_.warning_debounce_ms < config_.sustained_trip_duration_ms);

    if (!valid) {
        is_config_valid_ = false;
        // Clamp to safe defaults
        if (config_.accel_warning_clear >= config_.accel_warning_threshold) {
            config_.accel_warning_clear = (config_.accel_warning_threshold > 1000)
                                              ? (config_.accel_warning_threshold - 1000)
                                              : (config_.accel_warning_threshold / 2);
        }
        if (config_.motion_threshold >= config_.accel_warning_threshold) {
            config_.motion_threshold = config_.accel_warning_threshold / 2;
        }
        if (config_.warning_debounce_ms >= config_.sustained_trip_duration_ms) {
            config_.warning_debounce_ms = config_.sustained_trip_duration_ms / 2;
        }
    }
}

void VibrationMonitor::init()
{
    reset();
}

void VibrationMonitor::reset_window_extents()
{
    min_x_ = min_y_ = min_z_ = INT16_MAX;
    max_x_ = max_y_ = max_z_ = INT16_MIN;
}

void VibrationMonitor::reset()
{
    reset_window_extents();
    sample_count_ = 0;

    current_vib_ = 0;
    is_in_motion_ = false;
    is_warning_ = false;
    is_tripped_ = false;
    is_sensor_ok_ = false;
    trip_start_ms_ = 0;
    warning_since_ms_ = 0;
    last_sample_ms_ = 0;
    last_read_success_ms_ = 0;
}

void VibrationMonitor::set_accel_damping_mode(bool enabled)
{
    if (accel_damping_mode_ == enabled) {
        return;
    }
    accel_damping_mode_ = enabled;
    is_warning_ = false;
    warning_since_ms_ = 0;
}

void VibrationMonitor::update()
{
    if (!is_enabled_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();
    if (now - last_sample_ms_ < config_.sample_period_ms) {
        return;
    }
    last_sample_ms_ = now;

    hal::Vector3 sample;
    if (accel_.read_accel(sample)) {
        is_sensor_ok_ = true;
        last_read_success_ms_ = now;
        process_sample(sample, now);
    }
    else {
        if (now - last_read_success_ms_ > 200) {
            is_sensor_ok_ = false;
            current_vib_ = 0;
        }
    }
}

void VibrationMonitor::process_sample(const hal::Vector3& sample, uint32_t now)
{
    if (sample.x < min_x_) {
        min_x_ = sample.x;
    }
    if (sample.x > max_x_) {
        max_x_ = sample.x;
    }

    if (sample.y < min_y_) {
        min_y_ = sample.y;
    }
    if (sample.y > max_y_) {
        max_y_ = sample.y;
    }

    if (sample.z < min_z_) {
        min_z_ = sample.z;
    }
    if (sample.z > max_z_) {
        max_z_ = sample.z;
    }

    sample_count_++;
    if (sample_count_ >= config_.window_samples) {
        uint16_t dx = static_cast<uint16_t>(max_x_ - min_x_);
        uint16_t dy = static_cast<uint16_t>(max_y_ - min_y_);
        uint16_t dz = static_cast<uint16_t>(max_z_ - min_z_);

        current_vib_ = dx + dy + dz;

        // Reset tracking for next window
        reset_window_extents();
        sample_count_ = 0;

        // Evaluate motion
        is_in_motion_ = (current_vib_ >= config_.motion_threshold);

        // Evaluate warning with debounce and hysteresis (ACCEL-damping mode only)
        if (accel_damping_mode_) {
            if (current_vib_ >= config_.accel_warning_threshold) {
                if (warning_since_ms_ == 0) {
                    warning_since_ms_ = now;
                }
                if ((now - warning_since_ms_) >= config_.warning_debounce_ms) {
                    is_warning_ = true;
                }
            }
            else if (current_vib_ < config_.accel_warning_clear) {
                is_warning_ = false;
                warning_since_ms_ = 0;
            }
            else {
                // Hysteresis hold between clear and threshold:
                // if warning not latched, excursion below threshold resets debounce
                if (!is_warning_) {
                    warning_since_ms_ = 0;
                }
            }
        }
        else {
            is_warning_ = false;
            warning_since_ms_ = 0;
        }

        // Evaluate safety trip (active in all phases)
        if (current_vib_ >= config_.shock_threshold) {
            // Immediate single-shock trip (foot-slip zone)
            is_tripped_ = true;
            if (accel_damping_mode_) {
                is_warning_ = true;
            }
        }
        else if (current_vib_ >= config_.trip_threshold) {
            // Sustained trip zone
            if (trip_start_ms_ == 0) {
                trip_start_ms_ = now;
            }
            else if ((now - trip_start_ms_) >= config_.sustained_trip_duration_ms) {
                is_tripped_ = true;
            }
        }
        else {
            // Dropped below critical threshold - reset sustained timer
            trip_start_ms_ = 0;
        }
    }
}

} // namespace controllers
