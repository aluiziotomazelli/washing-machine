# Clean C++ Washing Machine Controller

[![CI - Host Tests & Firmware Build](https://github.com/aluiziotomazelli/washing-machine/actions/workflows/ci.yml/badge.svg)](https://github.com/aluiziotomazelli/washing-machine/actions/workflows/ci.yml)
[![Unit Tests](https://img.shields.io/badge/tests-203%20passed-brightgreen)](https://github.com/aluiziotomazelli/washing-machine)
[![Heap Allocation](https://img.shields.io/badge/heap-0%20bytes-blue)](https://github.com/aluiziotomazelli/washing-machine)
[![Target](https://img.shields.io/badge/target-ATmega328P%20%2F%2016MHz-orange)](https://github.com/aluiziotomazelli/washing-machine)
[![Coverage Report](https://img.shields.io/badge/coverage-report-blue)](https://aluiziotomazelli.github.io/washing-machine/index.html)

An industrial-grade, open-source custom controller firmware for domestic top-load washing machines. Built in modern, modular, non-blocking C++ for the ATmega328P microcontroller (Arduino Pro Mini / Nano), replacing obsolete or broken proprietary control boards.

---

## Key Features

* **Event-Driven Non-Blocking State Machine**: Zero `delay()` calls or blocking loops throughout the entire codebase. Every process runs concurrently with predictable timing.
* **Non-Volatile State Persistence & AC Power-Loss Recovery**: 64-slot wear-leveling circular ring buffer with CRC-8 data integrity validation in EEPROM. Automatically resumes interrupted wash cycles after power outages, intelligently distinguishes running vs. user-paused states (preventing unexpected midnight restarts during overnight soaks), and preserves user program/water level preferences.
* **Real-Time Out-of-Balance Sensing**: 50 Hz digital signal processing on the I2C bus via an MPU-6050 accelerometer with gravity offset rejection and peak-to-peak envelope windowing.
* **Multi-Tier Dynamic Unbalance Mitigation**:
  1. *Dry Coast-Down Retry*: Stops motor, keeps pump active for 10s until 0 RPM, and retries spin sprints.
  2. *Hydraulic Recovery*: Injects water to Low Level, executes a 30s agitation pattern to redistribute laundry evenly, drains, and smoothly resumes spin.
  3. *Latching Safety Trip*: Shuts down safely and alerts if unbalance persists.
* **Field Service Diagnostic Mode**: Built-in 7-step interactive hardware self-test routine for technicians—test individual water valves, drain pump, bi-directional motor, clutch, pressure switches, and live vibration VU-meter without opening the appliance.
* **Dual Visual Panel Support**:
  * **`main` branch**: WS2812B 9-pixel Addressable RGB LED strip with smooth breathing animations.
  * **`discrete-leds` branch**: Classical discrete LED panel board pinout.
* **Zero Dynamic Memory (0 Bytes Heap)**: Deterministic execution with zero heap fragmentation risk.
* **Dual-Target Native PC Unit Testing**: 203 unit tests written in GoogleTest/GoogleMock executing in ~45 ms on PC.
* **Hardware Watchdog Protection**: AVR hardware WDT with early boot disarm (`.init3`), continuous runtime kicking, and reboot detection with buzzer acoustic alerts.

---

## Hardware Pinout Reference

Pin assignments configured in [`src/hal/pinout.hpp`](src/hal/pinout.hpp) for the ATmega328P:

| Pin | Type | Function / Peripheral | Description |
| :---: | :---: | :--- | :--- |
| **D2** | Output | `k_valve_softener_pin` | Softener dispenser solenoid valve |
| **D3** | Output | `k_valve_main_pin` | Main water inlet dual solenoid valves |
| **D4** | Output | `k_drain_pump_pin` | Drain pump & mechanical brake clutch actuator |
| **D5** | Output | `k_buzzer_pin` | 3 kHz Piezo acoustic buzzer |
| **D6** | Output | `k_led_strip_pin` | WS2812B 9-Pixel Addressable RGB LED Strip DIN |
| **D7** | Input | `k_btn_softener_pin` | Extra Softener toggle button (Active-Low) |
| **D8** | Output | `k_motor_cw_pin` | Reversible Motor Clockwise (Right agitation / Spin) |
| **D9** | Output | `k_motor_ccw_pin` | Reversible Motor Counter-Clockwise (Left agitation) |
| **D10** | Input | `k_pressure_switch_low_pin` | Pressure Switch Contact 31-32 (NC, Low water level) |
| **D11** | Input | `k_pressure_switch_med_pin` | Pressure Switch Contact 11-13 (NO, Medium water level) |
| **D12** | Input | `k_pressure_switch_high_pin` | Pressure Switch Contact 21-23 (NO, High water level) |
| **A0 (14)** | Input | `k_btn_level_pin` | Water level selector button (Active-Low) |
| **A1 (15)** | Input | `k_btn_start_pin` | Start / Pause / Stage Advance button (Active-Low) |
| **A2 (16)** | Input | `k_btn_program_pin` | Wash program cycle button (Active-Low) |
| **A4 (18)** | I2C | `k_i2c_sda_pin` | MPU-6050 Accelerometer SDA |
| **A5 (19)** | I2C | `k_i2c_scl_pin` | MPU-6050 Accelerometer SCL |

---

## Codebase Architecture & Key Files

The codebase adheres strictly to SOLID principles, dependency injection, and clean separation between domain logic and hardware abstractions:

```
washing-machine/
├── washing-machine.ino                  # Entry point: dependency injection, peripheral setup & event loop
├── src/
│   ├── domain/                          # Core types, states, errors, programs, and stage enums
│   │   └── wash_types.hpp
│   ├── fsm/                             # Finite State Machine orchestration
│   │   └── wash_cycle_coordinator.hpp   # Master cycle recipe orchestrator & unbalance recovery coordinator
│   ├── controllers/                     # Atomic process controllers
│   │   ├── agitator.hpp                 # Non-blocking bi-directional agitation with dead-time safety
│   │   ├── fill_controller.hpp          # Water level monitoring and solenoid management with timeout safety
│   │   ├── drain_controller.hpp         # Water evacuation, empty detection, and smooth spin handover
│   │   ├── spin_controller.hpp          # Inertia sprint profiles, clutch timing, and deceleration
│   │   └── vibration_monitor.hpp        # 50 Hz DSP peak-to-peak windowing & unbalance trip logic
│   ├── hal/                             # Hardware Abstraction Layer (HAL)
│   │   ├── pinout.hpp                   # Physical GPIO and peripheral pinout definitions
│   │   ├── digital_output.hpp           # Actuator driver with initial-state safety
│   │   ├── reversible_motor.hpp         # Interlocked motor driver with 200 ms anti-shoot-through dead-time
│   │   ├── pressure_switch_sensor.hpp   # 3-level debounced electromechanical pressure switch reader
│   │   ├── mpu6050.hpp                  # Low-overhead I2C accelerometer driver with auto-detection
│   │   └── ws2812_strip.hpp             # Handcrafted AVR assembly 800 kHz zero-heap WS2812B driver
│   ├── persistence/                     # Non-volatile state persistence & wear leveling
│   │   ├── interfaces/
│   │   │   └── i_cycle_persistence.hpp  # Persistence interface contract
│   │   └── cycle_persistence_manager.hpp# 64-slot circular ring buffer with CRC-8 & wear leveling
│   └── ui/                              # User Interface & Diagnostics
│       ├── button.hpp                   # Debounce, short-click, long-click, and double-click state machine
│       ├── buzzer.hpp                   # Non-blocking acoustic melody and alert engine
│       ├── diagnostic_controller.hpp    # 7-step interactive technician diagnostic controller
│       ├── strip_led_panel.hpp          # WS2812B RGB visual presentation engine
│       └── discrete_led_panel.hpp       # Discrete GPIO LED visual presentation engine
├── test/                                # GoogleTest / GoogleMock PC unit test suite (200 tests)
└── docs/                                # In-depth documentation & engineering manuals
    ├── case-study.md                    # Complete engineering case study (from spaghetti to clean C++)
    └── technical-manual.md              # Field technician service & diagnostic manual
```

---

## Wash Programs & Controls

### Programs
1. **Normal Wash**: Main Fill -> 18 min Agitation -> Drain -> Intermediate Spin (2 min) -> Rinse -> Final Spin (4 min).
2. **Heavy Wash**: Main Fill -> 8 min Gentle Agitation -> 20 min Soak -> 14 min Normal Agitation -> Drain -> Intermediate Spin (2 min) -> Rinse -> Final Spin.
3. **Rinse Only**:
   * *Without Softener*: Fill -> 7 min Agitation -> Drain -> Final Spin.
   * *With Softener (Double Rinse)*: Fill -> 5 min Agitation -> Drain -> Intermediate Spin (2 min) -> Softener Fill -> 2 min Gentle Agitation -> 5 min Soak -> 2 min Post-Agitation -> Drain -> Final Spin.
4. **Spin Only**: Drain (with empty-tub fast-track) -> Sprints -> Continuous Cruise Spin (4 min).

### Button Controls
* **Start/Pause**:
  * *Click*: Start selected cycle / Pause running cycle / Resume paused cycle.
  * *Long Click*: Advance (skip) currently active stage to the next stage.
  * *Very Long Click*: Stop running cycle and resume to iddle.
  * **Obs:** Timing of clicks is defined in `washing-machine.ino` by `static ui::ButtonConfig btn_cfg{...}`
* **Program**: Cycle through Normal Wash -> Heavy Wash -> Rinse Only -> Spin Only.
* **Level**: Cycle water level between Low -> Medium -> High.
* **Softener**: Toggle single rinse vs. double rinse with softener dispenser.

---

## How to Build, Test & Flash

### On Linux / macOS (Recommended CLI Toolchain)

The project includes a comprehensive `Makefile` wrapping `arduino-cli` and `g++`:

```bash
# 1. Compile the Arduino AVR firmware:
make build

# 2. Upload firmware to the board via USB serial:
make flash PORT=/dev/ttyUSB0

# 3. Run all 200 native Host Unit Tests on your PC (GoogleTest / GoogleMock):
make test

# 4. Generate local HTML code coverage report (test/coverage/index.html):
make coverage

# 5. Generate compilation database for VS Code / IDE IntelliSense:
make compile-db
```

### In Arduino IDE
1. Open [`washing-machine.ino`](washing-machine.ino) in the Arduino IDE.
2. Select Board: **Arduino Pro or Pro Mini** (Processor: **ATmega328P, 5V, 16 MHz**).
3. Click **Verify** / **Upload**.

---

## 🔮 Future Roadmap & Planned Features

While the core firmware is production-ready and fully operational, the following enhancements are planned for future releases:

* **Linear Inductive Water Level Sensor (Frequency-Based)**:
  * Replace electromechanical pressure switches with an electronic linear frequency pressure sensor (e.g. LG 6501EA1001R / Samsung DC96-01703A).
  * Continuous real-time water column monitoring using a single GPIO Timer Input Capture / Interrupt (~21 kHz to ~27 kHz 5V square wave).
  * Eliminate mechanical switch calibration headaches, contact bounce, and physical hysteresis while enabling effortless software-defined level calibration and leak detection.
* **Energy-Saving Auto-Standby Mode**:
  * Dim or extinguish panel LEDs after 10 minutes of inactivity in `IDLE` state.
  * Instantly wake the UI on any button interaction.
* **Silent / Night Mode (Mute Buzzer)**:
  * Toggle acoustic alerts (e.g. via long press on the Softener button) for night-time wash operations.
  * Persist mute preference in EEPROM.

---

## Deep-Dive Documentation

* 📖 [**Architectural Case Study (`docs/case-study.md`)**](docs/case-study.md) — The complete engineering journey: memory optimization, hardware watchdog integration, out-of-balance DSP math, and refactoring timeline.
* 🛠️ [**Field Service Technical Manual (`docs/technical-manual.md`)**](docs/technical-manual.md) — Diagnostic step-by-step procedures, LED color codes, actuator test procedures, and troubleshooting charts.
* 📝 [**Historical 2017 Medium Article (Portuguese)**](https://medium.com/@aluiziotomazelli/controlando-uma-lavadora-de-roupas-com-arduino-b4aeb57a2abd) — The original mechanical/electrical restoration project.

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
