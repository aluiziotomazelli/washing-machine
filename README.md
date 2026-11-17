# Clean C++ Washing Machine Controller

[![CI - Host Tests & Firmware Build](https://github.com/aluiziotomazelli/washing-machine/actions/workflows/ci.yml/badge.svg)](https://github.com/aluiziotomazelli/washing-machine/actions/workflows/ci.yml)
[![Unit Tests](https://img.shields.io/badge/tests-205%20passed-brightgreen)](https://github.com/aluiziotomazelli/washing-machine)
[![Heap Allocation](https://img.shields.io/badge/heap-0%20bytes-blue)](https://github.com/aluiziotomazelli/washing-machine)
[![Target](https://img.shields.io/badge/target-ATmega328P%20%2F%2016MHz-orange)](https://github.com/aluiziotomazelli/washing-machine)
[![Coverage Report](https://img.shields.io/badge/coverage-report-blue)](https://aluiziotomazelli.github.io/washing-machine/index.html)

An open-source controller firmware for domestic top-load washing machines. It is written in modular, non-blocking C++ for the ATmega328P (Arduino Pro Mini / Nano) and replaces obsolete or broken proprietary control boards.

---

## Key Features

* **Event-driven, non-blocking state machine**: no blocking `delay()` calls in the control flow. Every task runs concurrently with predictable timing; the only `delay()` wrapper is in the HAL and no controller calls it.
* **Non-volatile state persistence and AC power-loss recovery**: a 64-slot wear-leveling ring buffer in EEPROM with CRC-8 validation. It resumes an interrupted wash cycle after a power outage, distinguishes a running cycle from one the user paused (so an overnight soak does not restart at 3 AM), and keeps the last program and water level.
* **Out-of-balance sensing**: an MPU-6050 on the I2C bus samples at 50 Hz. A rolling peak-to-peak window cancels the static gravity component.
* **Two-tier unbalance recovery**:
  1. *Dry retry*: stop the motor, keep the pump on for 10 s until the drum reaches 0 RPM, then retry the spin sprints.
  2. *Hydraulic recovery*: add water to Low Level, agitate for 30 s to redistribute the load, drain, then resume the spin.
  3. *Safety trip*: shut down and alert the user if the unbalance persists.
* **Field service diagnostic mode**: a 7-step self-test for technicians. It exercises the water valves, drain pump, reversible motor, clutch, pressure switches, and live vibration VU-meter from the panel, without opening the appliance.
* **Two panel options**: the `main` branch drives a 9-pixel WS2812B RGB strip with breathing animations; the `discrete-leds` branch keeps the classic discrete LED board pinout.
* **No dynamic memory**: the firmware uses 0 bytes of heap, so there is no fragmentation risk.
* **Dual-target tests**: 205 GoogleTest/GoogleMock tests run on a PC in about 50 ms.
* **Hardware watchdog**: the AVR WDT is disabled in `.init3` before C++ constructors run, kicked from the main loop, and reports a watchdog reset with a buzzer alert.

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
│   │   └── cycle_persistence_manager.hpp # 64-slot circular ring buffer with CRC-8 & wear leveling
│   └── ui/                              # User Interface & Diagnostics
│       ├── button.hpp                   # Debounce, short-click, long-click, and double-click state machine
│       ├── buzzer.hpp                   # Non-blocking acoustic melody and alert engine
│       ├── diagnostic_controller.hpp    # 7-step interactive technician diagnostic controller
│       ├── strip_led_panel.hpp          # WS2812B RGB visual presentation engine
│       └── discrete_led_panel.hpp       # Discrete GPIO LED visual presentation engine
├── test/                                # GoogleTest / GoogleMock PC unit test suite (205 tests)
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
  * *Hold >= 2.5 s*: Stop the running cycle and return to idle without releasing the button.
  * *Very Long Click*: Legacy stop-on-release behavior remains available.
  * **Obs:** Timing of clicks is defined in `washing-machine.ino` by `static ui::ButtonConfig btn_cfg{...}`
* **Program**: Cycle through Heavy Wash -> Normal Wash -> Rinse Only -> Spin Only.
* **Level**: Cycle water level between Low -> Medium -> High.
* **Softener**: Toggle single rinse vs. double rinse with softener dispenser.

---

## How to Build, Test & Flash

### On Linux / macOS (Recommended CLI Toolchain)

The `Makefile` wraps `arduino-cli` and `g++`:

```bash
# 1. Compile the Arduino AVR firmware:
make build

# 2. Upload firmware to the board via USB serial:
make flash PORT=/dev/ttyUSB0

# 3. Run the 205 host unit tests on your PC (GoogleTest / GoogleMock):
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

## Future roadmap

The current firmware works, and these features are planned:

* **Linear inductive water level sensor**: replace the electromechanical pressure switches with an electronic linear frequency sensor (LG 6501EA1001R / Samsung DC96-01703A). It would monitor the water column continuously on one GPIO using timer input capture (about 21 kHz to 27 kHz at 5 V), which removes switch calibration, contact bounce, and hysteresis, and allows software-defined level calibration and leak detection.
* **Auto-standby**: dim or turn off the panel LEDs after 10 minutes of inactivity in `IDLE`, and wake on any button press.
* **Silent mode**: toggle the buzzer with a long press on Softener, and store the preference in EEPROM.

---

## Deep-dive documentation

* [Architectural case study](docs/case-study.md): the engineering log from the 2017 rebuild to the current architecture. It covers memory use, the watchdog, the out-of-balance math, and the refactoring timeline.
* [Balance ring tuning](docs/balance_ring_analysis.md): fluid compensation data, conical precession math, and resonance telemetry from a Brastemp BWQ24B.
* [Field service manual](docs/technical-manual.md): diagnostic procedures, LED color codes, actuator tests, and troubleshooting charts.
* [Historical 2017 Medium article (Portuguese)](https://medium.com/@aluiziotomazelli/controlando-uma-lavadora-de-roupas-com-arduino-b4aeb57a2abd): the original mechanical and electrical restoration.

---

## License

This project is licensed under the [MIT License](LICENSE).
