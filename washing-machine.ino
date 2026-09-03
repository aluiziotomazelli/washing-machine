#include <Arduino.h>

#include "src/hal/pinout.hpp"
#include "src/hal/arduino/arduino_gpio_hal.hpp"
#include "src/hal/arduino/arduino_timer_hal.hpp"
#include "src/hal/arduino/arduino_watchdog_hal.hpp"
#include "src/hal/digital_output.hpp"
#include "src/hal/reversible_motor.hpp"
#include "src/hal/pressure_switch_sensor.hpp"
#include "src/ui/button.hpp"
#include "src/ui/buzzer.hpp"
#include "src/hal/ws2812_strip.hpp"
#include "src/ui/interfaces/i_button.hpp"
#include "src/ui/strip_led_panel.hpp"
#include "src/controllers/fill_controller.hpp"
#include "src/controllers/agitator.hpp"
#include "src/controllers/drain_controller.hpp"
#include "src/controllers/spin_controller.hpp"
#include "src/fsm/wash_cycle_coordinator.hpp"
#include "src/ui/panel_controller.hpp"
#include "src/hal/arduino/arduino_i2c_hal.hpp"
#include "src/hal/mpu6050.hpp"

// Hardware Abstraction Layer instances:
static hal::ArduinoGpioHAL gpio_hal;
static hal::ArduinoTimerHAL timer_hal;
static hal::ArduinoWatchdogHAL watchdog_hal;
static hal::ArduinoI2cHAL i2c_hal;

// Accelerometer Sensor:
static hal::Mpu6050 accel_sensor(i2c_hal);

// UI Hardware Components:
static ui::ButtonConfig btn_cfg{
    true, // active_low
    true, // enable_internal_pull
    20,   // debounce_press_ms
    20,   // debounce_release_ms
    100,  // double_click_ms
    800,  // long_click_ms
    1600, // very_long_click_ms
    6000  // timeout_ms
};

static ui::Button btn_start(gpio_hal, timer_hal, config::k_btn_start_pin, btn_cfg);
static ui::Button btn_program(gpio_hal, timer_hal, config::k_btn_program_pin, btn_cfg);
static ui::Button btn_level(gpio_hal, timer_hal, config::k_btn_level_pin, btn_cfg);
static ui::Button btn_softener(gpio_hal, timer_hal, config::k_btn_softener_pin, btn_cfg);

static hal::PressureSwitchConfig pressure_switch_cfg{
    {config::k_pressure_switch_low_pin, hal::ContactType::NORMALLY_CLOSED},
    {config::k_pressure_switch_med_pin, hal::ContactType::NORMALLY_OPEN},
    {config::k_pressure_switch_high_pin, hal::ContactType::NORMALLY_OPEN}};
static hal::PressureSwitchSensor water_level_sensor(gpio_hal, timer_hal, pressure_switch_cfg);

static ui::Buzzer buzzer(gpio_hal, timer_hal, config::k_buzzer_pin, 3000);

// Addressable RGB LED Strip Panel (9 Pixels WS2812B)
static hal::Ws2812Strip led_strip(config::k_led_strip_pin, 9);
static ui::StripLedPanel led_panel(led_strip, timer_hal);

// Actuators:
static hal::DigitalOutput valve_main(gpio_hal, config::k_valve_main_pin);
static hal::DigitalOutput valve_softener(gpio_hal, config::k_valve_softener_pin);
static hal::DigitalOutput drain_pump(gpio_hal, config::k_drain_pump_pin);
static hal::ReversibleMotor motor(gpio_hal, timer_hal, config::k_motor_cw_pin, config::k_motor_ccw_pin);

// Atomic Process Controllers:
static controllers::FillController fill_ctrl(timer_hal, valve_main, valve_softener, water_level_sensor);
static controllers::Agitator agitator(timer_hal, motor);
static controllers::DrainController drain_ctrl(timer_hal, drain_pump, water_level_sensor);
static controllers::SpinController spin_ctrl(timer_hal, drain_pump, motor);

// Central Cycle Coordinator (FSM):
static fsm::WashCycleCoordinator coordinator(timer_hal, fill_ctrl, agitator, drain_ctrl, spin_ctrl);

// UI Panel Controller:
static ui::PanelController panel_ctrl(btn_start, btn_program, btn_level, btn_softener, led_panel, buzzer, coordinator);

void setup()
{
    // Serial port for live telemetry (Arduino Serial Plotter compatible)
    Serial.begin(115200);

    // Check if recovery from hardware watchdog reset occurred
    bool recovered_from_wdt = watchdog_hal.was_reset_by_watchdog();

    // Initialize Hardware Peripherals
    btn_start.init();
    btn_program.init();
    btn_level.init();
    btn_softener.init();

    water_level_sensor.init();
    buzzer.init();
    led_panel.init();

    hal::DigitalOutput::init_all();
    motor.init();

    // Initialize MPU-6050 Accelerometer
    accel_sensor.init();

    // Arm Hardware Watchdog (2-second timeout protection against MCU freeze)
    watchdog_hal.enable(hal::WatchdogTimeout::TIMEOUT_2S);

    // Initialize Coordinator & UI Panel Presentation
    coordinator.init();
    panel_ctrl.init();

    // Audible alert if reboot was caused by Watchdog freeze recovery
    if (recovered_from_wdt) {
        buzzer.play_pattern(ui::BuzzerPattern::DOUBLE_BEEP);
    }
}

void loop()
{
    // Hardware sensor & driver periodic processing
    water_level_sensor.update();
    motor.update();

    // Process & UI coordination
    coordinator.update();
    panel_ctrl.update();

    // 50 Hz (every 20 ms) Live Telemetry for Arduino Serial Plotter
    static uint32_t last_telemetry_ms = 0;
    uint32_t now = timer_hal.get_time_ms();
    if (now - last_telemetry_ms >= 20) {
        last_telemetry_ms = now;

        hal::Vector3 accel;
        if (accel_sensor.read_accel(accel)) {
            // Sliding Peak-to-Peak envelope over 10 samples (200 ms)
            static int16_t min_x = 32767, max_x = -32768;
            static int16_t min_y = 32767, max_y = -32768;
            static int16_t min_z = 32767, max_z = -32768;
            static uint8_t sample_count = 0;
            static uint16_t vib_envelope = 0;

            if (accel.x < min_x) min_x = accel.x;
            if (accel.x > max_x) max_x = accel.x;
            if (accel.y < min_y) min_y = accel.y;
            if (accel.y > max_y) max_y = accel.y;
            if (accel.z < min_z) min_z = accel.z;
            if (accel.z > max_z) max_z = accel.z;

            sample_count++;
            if (sample_count >= 10) {
                uint16_t dx = static_cast<uint16_t>(max_x - min_x);
                uint16_t dy = static_cast<uint16_t>(max_y - min_y);
                uint16_t dz = static_cast<uint16_t>(max_z - min_z);
                // Total vibration magnitude (gravity DC bias cancelled out)
                vib_envelope = dx + dy + dz;

                min_x = 32767; max_x = -32768;
                min_y = 32767; max_y = -32768;
                min_z = 32767; max_z = -32768;
                sample_count = 0;
            }

            // Output format: X:val Y:val Z:val Vib:val
            Serial.print(F("X:")); Serial.print(accel.x);
            Serial.print(F(" Y:")); Serial.print(accel.y);
            Serial.print(F(" Z:")); Serial.print(accel.z);
            Serial.print(F(" Vib:")); Serial.print(vib_envelope);
            Serial.println();
        }
    }

    // Pet / kick hardware watchdog to prevent timeout reset
    watchdog_hal.kick();
}