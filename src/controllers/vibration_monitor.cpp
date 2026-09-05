#include "vibration_monitor.hpp"

namespace controllers {

VibrationMonitor::VibrationMonitor(
    hal::IAccelerometer& accelerometer,
    hal::ITimerHAL& timer_hal,
    const VibrationConfig& config
)
    : accel_(accelerometer)
    , timer_hal_(timer_hal)
    , config_(config)
{
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
    last_sample_ms_ = 0;
    last_read_success_ms_ = 0;
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
    } else {
        if (now - last_read_success_ms_ > 200) {
            is_sensor_ok_ = false;
            current_vib_ = 0;
        }
    }
}

void VibrationMonitor::process_sample(const hal::Vector3& sample, uint32_t now)
{
    last_sample_ = sample;

    if (sample.x < min_x_) min_x_ = sample.x;
    if (sample.x > max_x_) max_x_ = sample.x;

    if (sample.y < min_y_) min_y_ = sample.y;
    if (sample.y > max_y_) max_y_ = sample.y;

    if (sample.z < min_z_) min_z_ = sample.z;
    if (sample.z > max_z_) max_z_ = sample.z;

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

        // Evaluate warning
        is_warning_ = (current_vib_ >= config_.warning_threshold);

        // Evaluate safety trip
        if (current_vib_ >= config_.shock_threshold) {
            // Immediate single-shock trip (foot-slip zone)
            is_tripped_ = true;
        } else if (current_vib_ >= config_.trip_threshold) {
            // Sustained trip zone
            if (trip_start_ms_ == 0) {
                trip_start_ms_ = now;
            } else if ((now - trip_start_ms_) >= config_.sustained_trip_duration_ms) {
                is_tripped_ = true;
            }
        } else {
            // Dropped below critical threshold - reset sustained timer
            trip_start_ms_ = 0;
        }
    }
}

} // namespace controllers
