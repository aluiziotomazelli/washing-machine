#pragma once

#include <stdint.h>

namespace config {

// -----------------------------------------------------------------------------
// UI Input Pins (Buttons)
// -----------------------------------------------------------------------------
constexpr uint8_t k_btn_softener_pin = 1;  // TX / Softener toggle button
constexpr uint8_t k_btn_start_pin    = 17; // A3 (Pin 17) / Start-Stop button
constexpr uint8_t k_btn_level_pin    = 18; // A4 (Pin 18) / Water level cycle button
constexpr uint8_t k_btn_program_pin  = 19; // A5 (Pin 19) / Wash program cycle button

// -----------------------------------------------------------------------------
// Sensor Input Pins (Pressure Switch)
// -----------------------------------------------------------------------------
constexpr uint8_t k_pressure_switch_low_pin  = 10;  // NC contact 31-32 (Low water level)
constexpr uint8_t k_pressure_switch_med_pin  = 11;  // NO contact 11-13 (Medium water level)
constexpr uint8_t k_pressure_switch_high_pin = 255; // NO contact 21-23 (Unused on ATmega328P)

// -----------------------------------------------------------------------------
// UI Output Pins (Buzzer & LEDs)
// -----------------------------------------------------------------------------
constexpr uint8_t k_buzzer_pin        = 5;   // 3000 Hz Piezo transducer
constexpr uint8_t k_led_softener_pin  = 6;   // Softener function active LED
constexpr uint8_t k_led_power_pin     = 7;   // Machine running LED
constexpr uint8_t k_led_level_med_pin = 12;  // Medium water level LED
constexpr uint8_t k_led_level_low_pin = 13;  // Low water level LED
constexpr uint8_t k_led_spin_pin      = 14;  // A0 (Pin 14) / Spin stage LED
constexpr uint8_t k_led_rinse_pin     = 15;  // A1 (Pin 15) / Rinse stage LED
constexpr uint8_t k_led_wash_pin      = 16;  // A2 (Pin 16) / Wash stage LED

// -----------------------------------------------------------------------------
// Actuator Output Pins (Valves, Pump, Motor)
// -----------------------------------------------------------------------------
constexpr uint8_t k_valve_softener_pin = 2; // Solenoid 3 (Softener dispenser valve)
constexpr uint8_t k_valve_main_pin     = 3; // Solenoids 1 & 2 (Main water inlet valve)
constexpr uint8_t k_drain_pump_pin     = 4; // Drain pump & brake clutch actuator
constexpr uint8_t k_motor_cw_pin       = 8; // Motor clockwise rotation (Right)
constexpr uint8_t k_motor_ccw_pin      = 9; // Motor counter-clockwise rotation (Left)

} // namespace config
