#pragma once

#include <stdint.h>

namespace config {

// -----------------------------------------------------------------------------
// UI Input Pins (Pushbuttons with internal pull-up)
// -----------------------------------------------------------------------------
constexpr uint8_t k_btn_softener_pin = 1;  // TX (Pin 1) / Softener toggle button
constexpr uint8_t k_btn_start_pin    = 17; // A3 (Pin 17) / Start-Pause button
constexpr uint8_t k_btn_level_pin    = 18; // A4 (Pin 18) / Water level cycle button
constexpr uint8_t k_btn_program_pin  = 19; // A5 (Pin 19) / Wash program cycle button

// -----------------------------------------------------------------------------
// Sensor Input Pins (Pressure Switch)
// -----------------------------------------------------------------------------
constexpr uint8_t k_pressure_switch_low_pin  = 10;  // D10 / NC contact 31-32 (Low water level)
constexpr uint8_t k_pressure_switch_med_pin  = 11;  // D11 / NO contact 11-13 (Medium water level)
constexpr uint8_t k_pressure_switch_high_pin = 255; // NO contact 21-23 (Unused on discrete hardware)

// -----------------------------------------------------------------------------
// UI Output Pins (Buzzer & Discrete Indicator LEDs)
// -----------------------------------------------------------------------------
constexpr uint8_t k_buzzer_pin        = 5;   // D5 / 3000 Hz Piezo transducer
constexpr uint8_t k_led_softener_pin  = 6;   // D6 / Softener function active LED
constexpr uint8_t k_led_power_pin     = 7;   // D7 / Machine running LED
constexpr uint8_t k_led_level_low_pin = 13;  // D13 / Low water level LED
constexpr uint8_t k_led_level_med_pin = 12;  // D12 / Medium water level LED
constexpr uint8_t k_led_spin_pin      = 14;  // A0 (Pin 14) / Spin stage LED
constexpr uint8_t k_led_rinse_pin     = 15;  // A1 (Pin 15) / Rinse stage LED
constexpr uint8_t k_led_wash_pin      = 16;  // A2 (Pin 16) / Wash stage LED

// -----------------------------------------------------------------------------
// Actuator Output Pins (Valves, Pump, Motor)
// -----------------------------------------------------------------------------
constexpr uint8_t k_valve_softener_pin = 2; // D2 / Solenoid 3 (Softener dispenser valve)
constexpr uint8_t k_valve_main_pin     = 3; // D3 / Solenoids 1 & 2 (Main water inlet valve)
constexpr uint8_t k_drain_pump_pin     = 4; // D4 / Drain pump & brake clutch actuator
constexpr uint8_t k_motor_cw_pin       = 8; // D8 / Motor clockwise rotation (Right)
constexpr uint8_t k_motor_ccw_pin      = 9; // D9 / Motor counter-clockwise rotation (Left)

} // namespace config
