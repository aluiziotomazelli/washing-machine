#include <Arduino.h>

#include "src/hal/pinout.hpp"
#include "src/hal/arduino/arduino_gpio_hal.hpp"
#include "src/hal/arduino/arduino_timer_hal.hpp"
#include "src/hal/digital_output.hpp"
#include "src/hal/reversible_motor.hpp"
#include "src/hal/pressure_switch_sensor.hpp"
#include "src/ui/button.hpp"
#include "src/ui/buzzer.hpp"
#include "src/ui/discrete_led_panel.hpp"

// Hardware Abstraction Layer instances:
static hal::ArduinoGpioHAL gpio_hal;
static hal::ArduinoTimerHAL timer_hal;

// UI Components:
static ui::Button btn_softener(gpio_hal, timer_hal, config::k_btn_softener_pin);
static ui::Button btn_start(gpio_hal, timer_hal, config::k_btn_start_pin);
static ui::Button btn_level(gpio_hal, timer_hal, config::k_btn_level_pin);
static ui::Button btn_program(gpio_hal, timer_hal, config::k_btn_program_pin);

static hal::PressureSwitchConfig pressure_switch_cfg{
    {config::k_pressure_switch_low_pin, hal::ContactType::NORMALLY_CLOSED},
    {config::k_pressure_switch_med_pin, hal::ContactType::NORMALLY_OPEN},
    {config::k_pressure_switch_high_pin, hal::ContactType::NORMALLY_OPEN}
};
static hal::PressureSwitchSensor water_level_sensor(gpio_hal, timer_hal, pressure_switch_cfg);

static ui::Buzzer buzzer(gpio_hal, timer_hal, config::k_buzzer_pin, 3000);

static ui::DiscreteLedPins led_pins{
    config::k_led_power_pin,
    config::k_led_softener_pin,
    config::k_led_wash_pin,
    config::k_led_rinse_pin,
    config::k_led_spin_pin,
    config::k_led_level_low_pin,
    config::k_led_level_med_pin
};
static ui::DiscreteLedPanel led_panel(gpio_hal, led_pins);

// Actuators:
static hal::DigitalOutput valve_main(gpio_hal, config::k_valve_main_pin);
static hal::DigitalOutput valve_softener(gpio_hal, config::k_valve_softener_pin);
static hal::DigitalOutput drain_pump(gpio_hal, config::k_drain_pump_pin);
static hal::ReversibleMotor motor(gpio_hal, timer_hal, config::k_motor_cw_pin, config::k_motor_ccw_pin);

void setup()
{
    // Initialize UI and Sensors
    btn_softener.init();
    btn_start.init();
    btn_level.init();
    btn_program.init();

    water_level_sensor.init();
    buzzer.init();
    led_panel.init();

    // Initialize all actuators
    hal::DigitalOutput::init_all();
    motor.init();

    // Startup beep pattern
    buzzer.play_pattern(ui::BuzzerPattern::DOUBLE_BEEP);
}

void loop()
{
    // Periodic non-blocking input updates
    btn_softener.update();
    btn_start.update();
    btn_level.update();
    btn_program.update();
    water_level_sensor.update();

    // Periodic non-blocking output & driver updates
    buzzer.update();
    led_panel.update();
    motor.update();
}