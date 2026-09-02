#pragma once

#include <stdint.h>

namespace config {

// -----------------------------------------------------------------------------
// UI Input Pins (Pushbuttons with internal pull-up)
// -----------------------------------------------------------------------------
constexpr uint8_t k_btn_softener_pin = 7;  // D7 / Softener toggle button
constexpr uint8_t k_btn_program_pin  = 14; // A0 (Pin 14) / Wash program cycle button
constexpr uint8_t k_btn_start_pin    = 15; // A1 (Pin 15) / Start-Pause button
constexpr uint8_t k_btn_level_pin    = 16; // A2 (Pin 16) / Water level cycle button

// -----------------------------------------------------------------------------
// Sensor Input Pins (Pressure Switch)
// -----------------------------------------------------------------------------
constexpr uint8_t k_pressure_switch_low_pin  = 10; // D10 / NC contact 31-32 (Low water level)
constexpr uint8_t k_pressure_switch_med_pin  = 11; // D11 / NO contact 11-13 (Medium water level)
constexpr uint8_t k_pressure_switch_high_pin = 12; // D12 / NO contact 21-23 (High water level)

// -----------------------------------------------------------------------------
// UI Output Pins (Buzzer & WS2812 Addressable LED Strip)
// -----------------------------------------------------------------------------
constexpr uint8_t k_buzzer_pin    = 5; // D5 / 3000 Hz Piezo transducer
constexpr uint8_t k_led_strip_pin = 6; // D6 / WS2812B 9-Pixel DIN Data Line

// -----------------------------------------------------------------------------
// Actuator Output Pins (Valves, Pump, Motor)
// -----------------------------------------------------------------------------
constexpr uint8_t k_valve_softener_pin = 2; // D2 / Solenoid 3 (Softener dispenser valve)
constexpr uint8_t k_valve_main_pin     = 3; // D3 / Solenoids 1 & 2 (Main water inlet valve)
constexpr uint8_t k_drain_pump_pin     = 4; // D4 / Drain pump & brake clutch actuator
constexpr uint8_t k_motor_cw_pin       = 8; // D8 / Motor clockwise rotation (Right)
constexpr uint8_t k_motor_ccw_pin      = 9; // D9 / Motor counter-clockwise rotation (Left)

// -----------------------------------------------------------------------------
// Reserved Hardware Peripherals (I2C Bus & Serial UART)
// -----------------------------------------------------------------------------
constexpr uint8_t k_i2c_sda_pin = 18; // A4 (Pin 18) / I2C SDA (Accelerometer / Out-of-balance)
constexpr uint8_t k_i2c_scl_pin = 19; // A5 (Pin 19) / I2C SCL (Accelerometer / Out-of-balance)

} // namespace config
