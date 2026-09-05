#include "diagnostic_controller.hpp"

namespace ui {

DiagnosticController::DiagnosticController(
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
    controllers::IVibrationMonitor* vib_monitor)
    : btn_start_(btn_start)
    , btn_program_(btn_program)
    , led_panel_(led_panel)
    , buzzer_(buzzer)
    , water_sensor_(water_sensor)
    , valve_main_(valve_main)
    , valve_softener_(valve_softener)
    , drain_pump_(drain_pump)
    , motor_(motor)
    , timer_hal_(timer_hal)
    , vib_monitor_(vib_monitor)
    , agitator_(timer_hal, motor)
{
}

void DiagnosticController::enter()
{
    is_active_ = true;
    current_step_ = DiagnosticStep::LEVEL_SENSOR;
    stop_all_actuators();
    buzzer_.play_pattern(BuzzerPattern::DOUBLE_BEEP);
}

void DiagnosticController::exit()
{
    stop_all_actuators();
    is_active_ = false;
    led_panel_.turn_off_all();
    buzzer_.play_pattern(BuzzerPattern::DOUBLE_BEEP);
}

void DiagnosticController::stop_all_actuators()
{
    valve_active_ = false;
    pump_active_ = false;
    spin_active_ = false;
    spin_tripped_ = false;

    valve_main_.turn_off();
    valve_softener_.turn_off();
    drain_pump_.turn_off();
    if (agitator_.is_active()) {
        agitator_.stop();
    } else {
        motor_.stop();
    }
    motor_.update();
}

void DiagnosticController::next_step()
{
    stop_all_actuators();
    uint8_t next = static_cast<uint8_t>(current_step_) + 1;
    if (next >= static_cast<uint8_t>(DiagnosticStep::COUNT)) {
        next = 0;
    }
    current_step_ = static_cast<DiagnosticStep>(next);
    buzzer_.beep(50);
}

void DiagnosticController::update()
{
    if (!is_active_) {
        return;
    }

    // Check exit request (Long press on Start button)
    ButtonClickType start_click = btn_start_.get_last_click();
    if (start_click == ButtonClickType::LONG_CLICK || start_click == ButtonClickType::VERY_LONG_CLICK) {
        exit();
        return;
    }

    // Check step navigation (Click on Program button)
    ButtonClickType prog_click = btn_program_.get_last_click();
    if (prog_click == ButtonClickType::CLICK) {
        next_step();
    }

    // Execute active diagnostic step inspection
    switch (current_step_) {
    case DiagnosticStep::LEVEL_SENSOR:
        update_level_sensor_test();
        break;

    case DiagnosticStep::VIBRATION_SENSOR:
        update_vibration_sensor_test();
        break;

    case DiagnosticStep::MAIN_VALVE:
        update_main_valve_test(start_click);
        break;

    case DiagnosticStep::SOFTENER_VALVE:
        update_softener_valve_test(start_click);
        break;

    case DiagnosticStep::DRAIN_PUMP:
        update_drain_pump_test(start_click);
        break;

    case DiagnosticStep::MOTOR_AGITATE:
        update_motor_agitate_test(start_click);
        break;

    case DiagnosticStep::SPIN_TEST:
        update_spin_test(start_click);
        break;

    default:
        break;
    }
}

void DiagnosticController::update_level_sensor_test()
{
    domain::WaterLevel current_level = water_sensor_.get_current_level();
    led_panel_.show_diagnostic(
        DiagnosticStep::LEVEL_SENSOR,
        static_cast<uint16_t>(current_level),
        true
    );
}

void DiagnosticController::update_vibration_sensor_test()
{
    uint16_t vib = 0;
    bool ok = false;
    if (vib_monitor_ != nullptr) {
        vib_monitor_->update();
        vib = vib_monitor_->get_vibration();
        ok = vib_monitor_->is_sensor_ok();
    }
    led_panel_.show_diagnostic(
        DiagnosticStep::VIBRATION_SENSOR,
        vib,
        ok
    );
}

void DiagnosticController::update_main_valve_test(ButtonClickType start_click)
{
    if (start_click == ButtonClickType::CLICK) {
        valve_active_ = !valve_active_;
        buzzer_.beep(30);
    }

    // Overflow safety guard: auto shutoff if water level reaches HIGH_LEVEL
    if (valve_active_ && water_sensor_.get_current_level() == domain::WaterLevel::HIGH_LEVEL) {
        valve_active_ = false;
        buzzer_.play_pattern(BuzzerPattern::DOUBLE_BEEP);
    }

    if (valve_active_) {
        valve_main_.turn_on();
    } else {
        valve_main_.turn_off();
    }
    valve_softener_.turn_off();

    led_panel_.show_diagnostic(
        DiagnosticStep::MAIN_VALVE,
        valve_active_ ? 1 : 0,
        true
    );
}

void DiagnosticController::update_softener_valve_test(ButtonClickType start_click)
{
    if (start_click == ButtonClickType::CLICK) {
        valve_active_ = !valve_active_;
        buzzer_.beep(30);
    }

    // Overflow safety guard: auto shutoff if water level reaches HIGH_LEVEL
    if (valve_active_ && water_sensor_.get_current_level() == domain::WaterLevel::HIGH_LEVEL) {
        valve_active_ = false;
        buzzer_.play_pattern(BuzzerPattern::DOUBLE_BEEP);
    }

    valve_main_.turn_off();
    if (valve_active_) {
        valve_softener_.turn_on();
    } else {
        valve_softener_.turn_off();
    }

    led_panel_.show_diagnostic(
        DiagnosticStep::SOFTENER_VALVE,
        valve_active_ ? 1 : 0,
        true
    );
}

void DiagnosticController::update_drain_pump_test(ButtonClickType start_click)
{
    if (start_click == ButtonClickType::CLICK) {
        pump_active_ = !pump_active_;
        buzzer_.beep(30);
    }

    if (pump_active_) {
        drain_pump_.turn_on();
    } else {
        drain_pump_.turn_off();
    }

    domain::WaterLevel current_level = water_sensor_.get_current_level();
    led_panel_.show_diagnostic(
        DiagnosticStep::DRAIN_PUMP,
        static_cast<uint16_t>(current_level),
        pump_active_
    );
}

void DiagnosticController::update_motor_agitate_test(ButtonClickType start_click)
{
    if (start_click == ButtonClickType::CLICK) {
        if (agitator_.is_active()) {
            agitator_.stop();
        } else {
            agitator_.start(3600, 300, 200);
        }
        buzzer_.beep(30);
    }

    if (agitator_.is_active()) {
        agitator_.update();
    }
    motor_.update();

    hal::MotorState state = motor_.get_state();
    led_panel_.show_diagnostic(
        DiagnosticStep::MOTOR_AGITATE,
        static_cast<uint16_t>(state),
        true
    );
}

void DiagnosticController::update_spin_test(ButtonClickType start_click)
{
    if (start_click == ButtonClickType::CLICK) {
        if (spin_tripped_) {
            spin_tripped_ = false;
            buzzer_.beep(30);
        } else if (spin_active_) {
            spin_active_ = false;
            motor_.stop();
            drain_pump_.turn_off();
            buzzer_.beep(30);
        } else {
            spin_active_ = true;
            spin_start_time_ms_ = timer_hal_.get_time_ms();
            drain_pump_.turn_on();
            buzzer_.beep(30);
        }
    }

    if (spin_active_) {
        // Monitor vibration if available
        if (vib_monitor_ != nullptr) {
            vib_monitor_->update();
            if (vib_monitor_->is_critical_unbalance() || vib_monitor_->get_vibration() >= 11000) {
                spin_active_ = false;
                spin_tripped_ = true;
                motor_.stop();
                drain_pump_.turn_off();
                buzzer_.play_pattern(BuzzerPattern::DOUBLE_BEEP);
            }
        }

        if (spin_active_) {
            uint32_t elapsed = timer_hal_.get_time_ms() - spin_start_time_ms_;
            if (elapsed >= k_spin_clutch_delay_ms) {
                motor_.rotate_clockwise();
            }
        }
    }

    motor_.update();

    uint16_t raw_value = 0;
    if (drain_pump_.is_on()) {
        raw_value |= 0x01;
    }
    if (motor_.get_state() == hal::MotorState::RUNNING_CLOCKWISE) {
        raw_value |= 0x02;
    }

    led_panel_.show_diagnostic(
        DiagnosticStep::SPIN_TEST,
        raw_value,
        !spin_tripped_
    );
}

} // namespace ui
