#pragma once

#include <stdint.h>
#include "../domain/wash_types.hpp"
#include "../hal/interfaces/i_digital_output.hpp"
#include "../hal/interfaces/i_water_level_sensor.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace controllers {

/**
 * @class FillController
 * @brief Manages water filling sequence, controlling inlet valves and enforcing timeout protection.
 */
class FillController {
public:
    FillController(
        hal::ITimerHAL& timer_hal,
        hal::IDigitalOutput& valve_main,
        hal::IDigitalOutput& valve_softener,
        hal::IWaterLevelSensor& water_sensor,
        uint32_t timeout_ms = 720000 // 12 minutes default
    );

    void start(domain::WaterLevel target, bool use_softener = false);
    void update();
    void stop();

    bool is_active() const { return is_active_; }
    bool is_finished() const { return is_finished_; }
    bool has_error() const { return has_error_; }

private:
    hal::ITimerHAL& timer_hal_;
    hal::IDigitalOutput& valve_main_;
    hal::IDigitalOutput& valve_softener_;
    hal::IWaterLevelSensor& water_sensor_;
    uint32_t timeout_ms_;

    domain::WaterLevel target_level_{domain::WaterLevel::LOW_LEVEL};
    bool use_softener_{false};
    uint32_t start_time_ms_{0};

    bool is_active_{false};
    bool is_finished_{false};
    bool has_error_{false};
};

} // namespace controllers
