#include <Arduino.h>

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
static ui::Button btn_softener(gpio_hal, timer_hal, 1);
static ui::Button btn_start(gpio_hal, timer_hal, A3);
static ui::Button btn_level(gpio_hal, timer_hal, A4);
static ui::Button btn_program(gpio_hal, timer_hal, A5);

static hal::PressureSwitchSensor water_level_sensor(gpio_hal, timer_hal);
static ui::Buzzer buzzer(gpio_hal, timer_hal, 5, 3000);
static ui::DiscreteLedPanel led_panel(gpio_hal);

// Actuators:
static hal::DigitalOutput valve_main(gpio_hal, 3);
static hal::DigitalOutput valve_softener(gpio_hal, 2);
static hal::DigitalOutput drain_pump(gpio_hal, 4);
static hal::ReversibleMotor motor(gpio_hal, timer_hal, 8, 9);

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