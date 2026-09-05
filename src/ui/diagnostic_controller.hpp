#pragma once

#include <stdint.h>
#include "interfaces/i_button.hpp"
#include "interfaces/i_led_panel.hpp"
#include "interfaces/i_buzzer.hpp"
#include "../hal/interfaces/i_digital_output.hpp"
#include "../hal/interfaces/i_water_level_sensor.hpp"
#include "../hal/interfaces/i_reversible_motor.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"
#include "../controllers/interfaces/i_vibration_monitor.hpp"
#include "../controllers/agitator.hpp"

namespace ui {

using domain::DiagnosticStep;

/**
 * @class DiagnosticController
 * @brief Manages isolated bench testing and hardware diagnostics.
 *
 * Provides an in-situ self-test routine for technicians and field verification.
 */
class DiagnosticController
{
public:
    static constexpr uint32_t k_spin_clutch_delay_ms{5000};

    DiagnosticController(
        IButton& btn_start,
        IButton& btn_program,
        ILedPanel& led_panel,
        IBuzzer& buzzer,
        hal::IWaterLevelSensor& water_sensor,
        hal::IDigitalOutput& valve_main,
        hal::IDigitalOutput& valve_softener,
        hal::IDigitalOutput& drain_pump,
        hal::IReversibleMotor& motor,
        hal::ITimerHAL& timer_hal,
        controllers::IVibrationMonitor* vib_monitor = nullptr);

    void enter();
    void exit();
    void update();

    bool is_active() const { return is_active_; }
    DiagnosticStep get_current_step() const { return current_step_; }
    bool is_valve_active() const { return valve_active_; }
    bool is_pump_active() const { return pump_active_; }
    bool is_agitate_active() const { return agitator_.is_active(); }
    bool is_spin_active() const { return spin_active_; }
    bool is_spin_tripped() const { return spin_tripped_; }

private:
    void next_step();
    void stop_all_actuators();
    void update_level_sensor_test();
    void update_vibration_sensor_test();
    void update_main_valve_test(ButtonClickType start_click);
    void update_softener_valve_test(ButtonClickType start_click);
    void update_drain_pump_test(ButtonClickType start_click);
    void update_motor_agitate_test(ButtonClickType start_click);
    void update_spin_test(ButtonClickType start_click);

    IButton& btn_start_;
    IButton& btn_program_;
    ILedPanel& led_panel_;
    IBuzzer& buzzer_;
    hal::IWaterLevelSensor& water_sensor_;
    hal::IDigitalOutput& valve_main_;
    hal::IDigitalOutput& valve_softener_;
    hal::IDigitalOutput& drain_pump_;
    hal::IReversibleMotor& motor_;
    hal::ITimerHAL& timer_hal_;
    controllers::IVibrationMonitor* vib_monitor_;

    controllers::Agitator agitator_;

    DiagnosticStep current_step_{DiagnosticStep::LEVEL_SENSOR};
    bool is_active_{false};
    bool valve_active_{false};
    bool pump_active_{false};
    bool spin_active_{false};
    bool spin_tripped_{false};
    uint32_t spin_start_time_ms_{0};
};

} // namespace ui
