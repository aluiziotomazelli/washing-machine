# 🧺 From Spaghetti to Clean Embedded C++: The Washing Machine Case Study

> A deep-dive engineering log documenting the architectural refactoring of a real-world domestic appliance firmware: transitioning from a monolithic legacy Arduino prototype into a modern, test-driven, decoupled embedded C++ architecture.

---

## 📑 Table of Contents
1. [Introduction & Background](#introduction--background)
2. [Chapter 1: Deconstructing the Legacy Codebase (v0.1.0)](#chapter-1-deconstructing-the-legacy-codebase-v010)
3. [Chapter 2: Hardware Abstraction & Dependency Injection](#chapter-2-hardware-abstraction--dependency-injection)
4. [Chapter 3: Dual-Target Development with GoogleTest, Google Mock & Automated CI](#chapter-3-dual-target-development-with-googletest-google-mock--automated-ci)
5. [Chapter 4: Actuator Safety, Motor Dead-Time & Peripheral Drivers](#chapter-4-actuator-safety-motor-dead-time--peripheral-drivers)
6. [Chapter 5: Domain Modeling & Atomic Process Controllers (SRP)](#chapter-5-domain-modeling--atomic-process-controllers-srp)
7. [Chapter 6: Event-Driven Wash Cycle Coordinator FSM & Clean UI Architecture](#chapter-6-event-driven-wash-cycle-coordinator-fsm--clean-ui-architecture)
8. [Chapter 7: The WS2812B Addressable LED Engine, Hardware Re-spin & The AVR Interrupt Collision Dilemma](#chapter-7-the-ws2812b-addressable-led-engine-hardware-re-spin--the-avr-interrupt-collision-dilemma)
9. [Chapter 8: Safety Watchdogs, Out-of-Balance Sensing & I2C Vibration *(Upcoming)*](#chapter-8-safety-watchdogs-out-of-balance-sensing--i2c-vibration)
10. [Chapter 9: Hardware Migration to 32-bit Architecture (ESP32-C3) *(Upcoming)*](#chapter-9-hardware-migration-to-32-bit-architecture-esp32-c3)

---

## Introduction & Background

In 2017, a domestic washing machine suffered a catastrophic failure of its proprietary electronic control board. Rather than discarding the machine, the electronics were reverse-engineered and replaced with an **ATmega328P microcontroller board (Arduino Pro Mini, 5V, 16MHz)** driving TRIACs and power relays.

The initial prototype proved that open-source hardware could successfully breathe new life into household machinery. However, written as a quick single-file `.ino` sketch, the code carried significant technical debt.

This case study documents the transformation of that monolithic prototype into a **production-grade, testable, object-oriented embedded C++ system**.

---

## Chapter 1: Deconstructing the Legacy Codebase (v0.1.0)

Before refactoring, we analyzed the code smells and architectural bottlenecks in [`washing-machine.ino`](../washing-machine.ino):

### 1. The Monolithic `.ino` Anti-Pattern
All domain logic, hardware pin assignments, timing loops, UI debouncers, and safety timeouts lived in a single 800+ line file. Modifying one feature (e.g. adjusting agitation speed) risked breaking unrelated subsystems (e.g. water fill detection).

### 2. Blocking `delay()` and `while()` Traps
The firmware relied heavily on blocking loops:
```cpp
// Legacy blocking agitation loop:
while (now() <= tempoAvanco) {
    digitalWrite(motorDir, HIGH);
    delay(300);
    digitalWrite(motorDir, LOW);
    delay(200); // Motor dead-time
    digitalWrite(motorEsq, HIGH);
    delay(300);
    digitalWrite(motorEsq, LOW);
    delay(200);
}
```
**Why this is dangerous:** While the processor is stuck inside a `delay()` call, the CPU is 100% blind. It cannot monitor safety sensors, process emergency button presses, read accelerometer vibration interrupts, or update visual indicators smoothly.

### 3. Global Mutable State
Variables like `seletorPrograma`, `usaAmaciante`, `estadoChaveNivelB`, and `erro` were global. Any function could modify any state without validation, making the system prone to race conditions and difficult to reason about.

### 4. Zero Automated Testability
Because the code directly called Arduino hardware functions (`digitalRead`, `digitalWrite`, `pinMode`), it was impossible to run on a PC. Every test required uploading to physical hardware and waiting real minutes for cycles to complete.

---

## Chapter 2: Hardware Abstraction & Dependency Injection

To make the codebase testable and portable, we established the **Hardware Abstraction Layer (HAL)**.

```mermaid
classDiagram
    class IGpioHAL {
        <<interface>>
        +set_mode(pin, mode)*
        +set_level(pin, level)*
        +get_level(pin)*
        +play_tone(pin, freq)*
        +stop_tone(pin)*
    }
    class ITimerHAL {
        <<interface>>
        +get_time_ms()*
        +get_time_us()*
        +delay_ms(ms)*
    }
    class ArduinoGpioHAL {
        +set_mode(pin, mode)
        +set_level(pin, level)
        +get_level(pin)
        +play_tone(pin, freq)
        +stop_tone(pin)
    }
    class ArduinoTimerHAL {
        +get_time_ms()
        +get_time_us()
        +delay_ms(ms)
    }
    class MockGpioHAL {
        +MOCK_METHOD()
    }
    class MockTimerHAL {
        +MOCK_METHOD()
    }
    
    IGpioHAL <|-- ArduinoGpioHAL
    IGpioHAL <|-- MockGpioHAL
    ITimerHAL <|-- ArduinoTimerHAL
    ITimerHAL <|-- MockTimerHAL
```

### Key Design Decisions:

1. **Pure Abstract Interfaces:**
   - [`src/hal/interfaces/i_gpio_hal.hpp`](../src/hal/interfaces/i_gpio_hal.hpp): Encapsulates pin direction, digital I/O, and hardware tone generation.
   - [`src/hal/interfaces/i_timer_hal.hpp`](../src/hal/interfaces/i_timer_hal.hpp): Encapsulates system timestamp services (`millis`, `micros`).

2. **Constructor Dependency Injection via References (`&`):**
   Classes receive their dependencies as C++ references:
   ```cpp
   Button(hal::IGpioHAL& gpio_hal,
          hal::ITimerHAL& timer_hal,
          uint8_t pin,
          const ButtonConfig& config = ButtonConfig{});
   ```
   - **Guaranteed Non-Null:** Eliminates null pointer checks.
   - **Zero Heap Overhead:** No dynamic allocation (`new`/`malloc`/`shared_ptr`), critical for 2 KB SRAM microcontrollers.

3. **Macro Collision Avoidance:**
   Legacy Arduino headers define `#define HIGH 0x1`, `#define INPUT 0x0`. To prevent the C preprocessor from corrupting modern C++ `enum class` declarations, enum values use explicit scoped names: `hal::GpioMode::MODE_INPUT`, `hal::GpioLevel::LEVEL_HIGH`.

---

## Chapter 3: Dual-Target Development with GoogleTest, Google Mock & Automated CI

Rather than postponing testing to a late milestone, we adopted **Dual-Target Development** directly in `v0.2.0`. Every driver and state machine is verified on the host machine in milliseconds.

```mermaid
graph LR
    A[C++ Driver Code] --> B{Target Compilation}
    B -->|g++ / Linux| C[GoogleTest & GoogleMock Test Runner]
    B -->|avr-gcc / Arduino| D[ATmega328P Flash Binary]
    C -->|~18 ms| E[34 Automated Unit Tests Passed]
    D -->|~10 s| F[Hardware Deployment]
```

### 1. The Finite State Machine `Button` Driver
We created a robust, non-blocking [`ui::Button`](../src/ui/button.hpp) driven by a 6-state FSM:
- **`WAIT_FOR_PRESS`**: Idle waiting for active level.
- **`DEBOUNCE_PRESS`**: Filters electrical noise glitches (< 20 ms).
- **`WAIT_FOR_RELEASE`**: Measures press duration (distinguishing single click, long press > 1000 ms, and very long press > 3000 ms).
- **`DEBOUNCE_RELEASE`**: Confirms clean release.
- **`WAIT_FOR_DOUBLE`**: 300 ms window to detect double clicks.
- **`TIMEOUT_WAIT_FOR_RELEASE`**: Protects against physically stuck buttons (> 6000 ms).

### 2. Automated Continuous Integration (GitHub Actions)
We configured [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) with automatic caching (`actions/cache@v4`):
- **Host Tests:** Native GoogleTest and Google Mock compile and run all unit tests in **~18 ms**.
- **AVR Build:** `arduino-cli` compiles the full ATmega328P target binary in **~10 s**.

---

## Chapter 4: Actuator Safety, Motor Dead-Time & Peripheral Drivers

Controlling high-power AC inductive loads (wash motor, drain pump, brake clutch, solenoid valves) and user interface peripherals requires strict safety interlocks.

```mermaid
stateDiagram-v2
    [*] --> STOPPED
    
    STOPPED --> RUNNING_CLOCKWISE: rotate_clockwise()
    STOPPED --> RUNNING_COUNTER_CLOCKWISE: rotate_counter_clockwise()
    
    RUNNING_CLOCKWISE --> STOPPED: stop()
    RUNNING_COUNTER_CLOCKWISE --> STOPPED: stop()
    
    RUNNING_CLOCKWISE --> DEAD_TIME_WAIT: rotate_counter_clockwise() [Turn OFF CW]
    RUNNING_COUNTER_CLOCKWISE --> DEAD_TIME_WAIT: rotate_clockwise() [Turn OFF CCW]
    
    DEAD_TIME_WAIT --> RUNNING_COUNTER_CLOCKWISE: After dead_time_ms expires
    DEAD_TIME_WAIT --> RUNNING_CLOCKWISE: After dead_time_ms expires
    DEAD_TIME_WAIT --> STOPPED: stop() [Emergency cancel]
```

### 1. General Binary Loads: `DigitalOutput` & Intrusive Linked List
Single-pin binary actuators (inlet valves, drain pump, clutch actuator) are controlled via [`hal::DigitalOutput`](../src/hal/digital_output.hpp):
- **Intrusive Linked List Pattern:** Objects register themselves into a static linked list during construction without using dynamic memory (`new` / `malloc` / `std::vector`), allowing global batch operations (`DigitalOutput::init_all()`, `DigitalOutput::turn_off_all()`).
- **Active-Low vs Active-High Support:** Seamlessly drives direct-logic outputs and inverted relay modules.
- **Safety Decision (Exclusion of `turn_on_all`):** Turning on all physical loads simultaneously in an appliance (filling valves, heater, pump, and spin motor at once) could cause power surges or domestic breaker trips. Thus, only batch *turn-off* (`turn_off_all()`) is supported for fail-safe emergency shutdowns.

### 2. Bidirectional AC Motor: `ReversibleMotor`
A reversible AC induction motor possesses two independent windings (Clockwise / Right and Counter-Clockwise / Left). Energizing both windings simultaneously creates a violent phase short-circuit across the TRIACs/relays.

[`hal::ReversibleMotor`](../src/hal/reversible_motor.hpp) enforces safety through software:
1. **Mutual Exclusion:** Before setting either direction pin to active level, the opposing direction pin is unconditionally forced to `LEVEL_LOW`.
2. **Non-Blocking Dead-Time:** When reversing from Clockwise to Counter-Clockwise (or vice-versa), both pins are immediately de-energized, and the driver enters `DEAD_TIME_WAIT`. The new direction is energized only after the configured dead-time window (e.g. 200 ms) has elapsed.
3. **Emergency Stop Override:** Calling `stop()` during a dead-time window immediately aborts the pending rotation, ensuring the motor stays safely stopped.

### 3. Water Level Sensing: `PressureSwitchSensor`
The physical electromechanical pressure switch features mixed contact topologies:
- **Low Level (31-32):** Normally Closed (NC) $\rightarrow$ opens under pressure (`LEVEL_HIGH` when level reached).
- **Medium Level (11-13):** Normally Open (NO) $\rightarrow$ closes under pressure (`LEVEL_LOW` when level reached).
- **High Level (21-23):** Normally Open (NO) $\rightarrow$ closes under pressure (`LEVEL_LOW` when level reached).

[`hal::PressureSwitchSensor`](../src/hal/pressure_switch_sensor.hpp) normalizes these raw polarities and implements a **100 ms hydraulic stabilization filter** (*sloshing debouncer*) to prevent false triggering from water waves.

### 4. Non-Blocking Audio: `Buzzer` (Passive Transducer Engine)
The appliance uses a passive piezoelectric transducer without an internal oscillator. [`ui::Buzzer`](../src/ui/buzzer.hpp) utilizes non-blocking 3000 Hz hardware tone generation (`play_tone` / `stop_tone`) to produce acoustic cues:
- `SHORT_BEEP` (50 ms button click feedback)
- `DOUBLE_BEEP` (function toggle)
- `CYCLE_FINISHED` (4 loud, repetitive finish beeps to alert from afar)
- `ERROR_ALARM` (continuous alternating alarm)

### 5. Decoupled Visual Feedback: `ILedPanel` & `DiscreteLedPanel`
To allow transitioning to addressable RGB LEDs (WS2812B) in the future without modifying domain logic, [`ui::ILedPanel`](../src/ui/interfaces/i_led_panel.hpp) provides a semantic visual interface (`set_stage(WashStage)`, `set_selected_level(WaterLevel)`, `set_softener(bool)`). The current implementation [`ui::DiscreteLedPanel`](../src/ui/discrete_led_panel.hpp) drives the 7 physical panel LEDs.

---

## Chapter 5: Domain Modeling & Atomic Process Controllers (SRP)

As the project moved toward cycle orchestration, we confronted a critical architectural decision: how to coordinate complex physical actions (filling, agitating, draining, spinning) without creating an unmaintainable monolithic "God Class".

```mermaid
graph TD
    subgraph Domain_Layer ["Core Domain (src/domain/)"]
        DT["wash_types.hpp<br>(WaterLevel, WashProgram, WashStage, MachineState)"]
    end

    subgraph Process_Controllers ["Atomic Process Controllers (src/controllers/)"]
        FC["FillController<br>(Valves + 12m Timeout + Pause)"]
        AC["Agitator<br>(CW/CCW 300ms/200ms Stroke Machine)"]
        DC["DrainController<br>(Pump + 30s Bleed + 6m Timeout)"]
        SC["SpinController<br>(Clutch + Sprints + 4s/4s + Coast-Down)"]
    end

    subgraph Hardware_UI ["HAL & UI Adapters"]
        HAL["HAL Drivers (Motor, Relays, Sensor)"]
        UI["UI Drivers (LedPanel, Buzzer, Buttons)"]
    end

    FC --> DT
    AC --> DT
    DC --> DT
    SC --> DT
    HAL --> DT
    UI --> DT
    FC --> HAL
    AC --> HAL
    DC --> HAL
    SC --> HAL
```

### 1. Eliminating Coupling Inversion with Domain Types
In early prototypes, enums like `WashProgram` and `WashStage` were defined in `i_led_panel.hpp`, and `WaterLevel` was in `i_water_level_sensor.hpp`. This inverted dependency forced central business logic to include UI and sensor headers.

We extracted all core concepts into [`src/domain/wash_types.hpp`](../src/domain/wash_types.hpp) in namespace `domain`. The domain is now completely pure: HAL, UI, and business logic depend solely on the domain model.

### 2. Deconstructing the "God Class" via SRP
Rather than handling solenoid timings, motor reversals, and drain timeouts inside one massive coordinator, each physical operation was isolated into an **Atomic Process Controller** with a unified lifecycle:

| Controller | Single Responsibility | Key Features |
| :--- | :--- | :--- |
| [`FillController`](../src/controllers/fill_controller.hpp) | Tub water intake | Controls main/softener solenoids; 12-min fail-safe timeout; `pause()` freezes timeout counter. |
| [`Agitator`](../src/controllers/agitator.hpp) | Mechanical fabric wash | 300 ms ON / 200 ms OFF stroke alternation; non-blocking CW/CCW reversal; preserves progress on `pause()`. |
| [`DrainController`](../src/controllers/drain_controller.hpp) | Water evacuation | Controls pump; detects empty tub; executes 30-sec residual water bleeding; 6-min timeout; `pause()` freezes bleeding. |
| [`SpinController`](../src/controllers/spin_controller.hpp) | Centrifugal water extraction | 5s clutch engagement; volume-based sprints; 4s ON / 4s OFF duty cycle; non-blocking transmission protection. |

### 3. Deep-Dive: Top-Load Transmission Mechanics & Inertia Management
Domestic top-load washing machine transmissions (e.g. Whirlpool/Brastemp/Consul) have unique mechanical constraints:
- **Agitation Mode (Actuator OFF):** A heavy spring clamps the brake band onto the outer tub campana, keeping the tub stationary while the central shaft freely oscillates the agitator.
- **Spin Mode (Actuator ON):** The electromechanical actuator pulls a mechanical arm, opening the brake band and meshing the ratchet clutch teeth. The entire drum spins at 700+ RPM.

#### The "Violent Brake-Snap" Trap:
If electrical power to the pump/actuator is cut while the drum is spinning at full speed, the brake spring instantly snaps the brake band onto the steel drum. This produces a loud mechanical crash ("tranco"), severely wearing the brake lining, straining the belt, and risking tooth shear on the plastic clutch came.

#### The Software Solution: Non-Blocking `COAST_DOWN` Protection
In [`SpinController`](../src/controllers/spin_controller.hpp), both normal cycle completion, `pause()`, and `stop()` pass through a 10-second `COAST_DOWN` phase:
1. Power to the motor is cut immediately.
2. The drain pump/actuator **remains energized** for 10 seconds, allowing the drum to decelerate smoothly through air resistance and natural friction.
3. Only when the drum has settled does the controller de-energize the actuator, letting the brake engage quietly without mechanical shock.

#### Rotational Inertia Duty Cycle (4s ON / 4s OFF):
Continuous spin does not energize the single-phase AC induction motor non-stop. After the initial sprint ramp-down, it runs a **4000 ms ON / 4000 ms OFF** duty cycle. The high rotational momentum ($J \cdot \omega$) keeps the basket spinning at extraction velocity while cutting thermal load and electrical consumption by 50%.

### 4. Dual-Target Test Growth
With atomic process controllers, each physical action is tested independently with GoogleTest & GoogleMock:
- `FillControllerTest` (6 tests): level cut-off, timeout detection, valve control, pause/resume.
- `AgitatorTest` (4 tests): CW/CCW alternation, off-pause timing, duration completion, pause/resume.
- `DrainControllerTest` (5 tests): empty detection, 30s bleed phase, timeout detection, pause/resume.
- `SpinControllerTest` (7 tests): clutch engage, level-dependent sprints, 4s/4s duty run, coast-down, soft pause, soft stop, emergency stop.
- `WashCycleCoordinatorTest` (8 tests): recipe sequencing, step advancement, error triggers, process delegation.
- `PanelControllerTest` (12 tests): button event handling, UI state transitions, audio feedback, differentiated error diagnostics.

**Total Host Tests:** **80 automated unit tests passing in ~42 ms**.

---

## Chapter 6: Event-Driven Wash Cycle Coordinator & UI Panel Orchestration

With the atomic process controllers established, the final milestone of `v0.3.0` was building the high-level **Orchestrator (FSM Coordinator)** and the **Panel Controller**, tying the entire system together via Dependency Injection.

```mermaid
graph TD
    subgraph UI_Layer ["Presentation & UI Layer (src/ui/)"]
        BTN["4x Non-Blocking Buttons<br>(Start, Program, Level, Softener)"]
        PANEL["PanelController<br>(Event Translation & UI Feedback)"]
        DISCRETE["DiscreteLedPanel<br>(LED Animations & Errors)"]
        BUZZER["Buzzer<br>(Acoustic Patterns)"]
    end

    subgraph Coordination_Layer ["FSM Orchestration (src/fsm/)"]
        COORD["WashCycleCoordinator<br>(Recipe Sequencer: Normal, Heavy, Rinse, Spin)"]
    end

    subgraph Process_Layer ["Process Controllers (src/controllers/)"]
        FILL["FillController"]
        AGIT["Agitator"]
        DRAIN["DrainController"]
        SPIN["SpinController"]
    end

    BTN -->|Click Events| PANEL
    PANEL -->|Start, Pause, Resume, Advance, Stop| COORD
    COORD -->|State & Stage Sync| PANEL
    PANEL -->|Visual Feedback| DISCRETE
    PANEL -->|Acoustic Feedback| BUZZER
    COORD -->|Delegates Step| FILL
    COORD -->|Delegates Step| AGIT
    COORD -->|Delegates Step| DRAIN
    COORD -->|Delegates Step| SPIN
```

### 1. The `WashCycleCoordinator` Recipe Sequencer
The [`WashCycleCoordinator`](../src/fsm/wash_cycle_coordinator.hpp) implements the complete laundry recipes without blocking:
- **Normal Wash:** Fill $\rightarrow$ Continuous Agitation (18m) $\rightarrow$ Drain $\rightarrow$ Rinse $\rightarrow$ Spin.
- **Heavy Wash:** Fill $\rightarrow$ Gentle Agitation (8m) $\rightarrow$ Long Soak (20m) $\rightarrow$ Normal Agitation (14m) $\rightarrow$ Drain $\rightarrow$ Rinse $\rightarrow$ Spin.
- **Rinse Only:** Single Rinse (no softener) or Double Rinse with Softener (Fill $\rightarrow$ Agitate $\rightarrow$ Drain $\rightarrow$ Interm Spin $\rightarrow$ Softener Fill $\rightarrow$ Agitate $\rightarrow$ Softener Soak $\rightarrow$ Post Agitate $\rightarrow$ Drain $\rightarrow$ Final Spin).
- **Spin Only:** Drain $\rightarrow$ 30s Bleed $\rightarrow$ Clutch Sprints $\rightarrow$ Continuous 4s/4s Spin $\rightarrow$ Coast-Down.

### 2. The `PanelController` Presentation Layer
The [`PanelController`](../src/ui/panel_controller.hpp) bridges user input with the domain:
- **Zero Polling in Domain:** Translates single clicks, long clicks (step advance), and very long clicks (cycle cancel) into coordinator commands.
- **Differentiated Error Diagnostics (Legacy Restored):**
  - `FILL_TIMEOUT` (12-min inlet fail-safe): Blinks all water level LEDs.
  - `DRAIN_TIMEOUT` (5-min pump fail-safe): Blinks all program stage LEDs.
- **Non-Blocking Visual & Acoustic Feedback:** Synchronizes solid vs blinking power LEDs, stage progress, and tone patterns smoothly.

### 3. Main System Assembly via Inversion of Control (`washing-machine.ino`)
The top-level Arduino sketch contains zero business logic, acting strictly as the **Composition Root**:
```cpp
void setup() {
    // 1. Initialize hardware I/O pins
    // 2. Initialize UI presentation
    panel_ctrl.init();
}

void loop() {
    // Non-blocking tick pump
    btn_start.update();
    btn_program.update();
    btn_water_level.update();
    btn_softener.update();

    coordinator.update();
    panel_ctrl.update();
    led_panel.update();
    buzzer.update();
}
```

### 4. Memory Footprint on ATmega328P (5V, 16MHz)
- **Flash ROM:** 14,976 bytes (**48%** of 30,720 bytes).
- **SRAM:** 721 bytes (**35%** of 2,048 bytes).
- **Dynamic Allocation (`heap`):** 0 bytes (100% static allocation).

---

## Chapter 7: The WS2812B Addressable LED Engine, Hardware Re-spin & The AVR Interrupt Collision Dilemma

With the discrete LED release (`v0.3.1`) validated and archived, the physical control panel (wooden box) presented a compelling mechanical and visual opportunity: replacing the complex harness of 7 discrete through-hole LEDs with a single, compact **9-pixel WS2812B addressable RGB strip**.

This upgrade introduced fascinating embedded challenges spanning cycle-accurate AVR assembly, real-world analog voltage levels, and timer interrupt collisions.

```
Physical Layout of the 9-Pixel Strip (docs/box.webp):
[ P8 ]       [ P7 ]       [ P6 ]   [ P5 ]   [ P4 ]       [ P3 ]       [ P2 ]   [ P1 ]   [ P0 ]
Amac.        Vazio        Baixo    Médio    Alto         Pesado       Lavar    Enxag.   Centrif.
(Rose)       (OFF)       (Cyan)   (Cyan)   (Cyan)       (White)      (White)  (White)  (White)
  ◄────────────────────────────────── Physical Strip Orientation ────────────────────────────── DIN
```

---

### 1. The Zero-Heap 16 MHz AVR Assembly Driver (`Ws2812Strip`)

Standard third-party libraries (like FastLED or Adafruit NeoPixel) bring significant flash overhead, dynamic memory allocations, and monolithic dependencies unsuitable for a safety-critical appliance on an ATmega328P.

We engineered a bespoke, zero-heap hardware driver [`Ws2812Strip`](../src/hal/ws2812_strip.hpp) adhering to Clean Architecture:
- **Static Buffer:** Exactly $9 \times 3 = 27$ bytes in GRB color order. Zero heap allocation (`malloc` free).
- **Nanosecond-Accurate Inline Assembly:** The WS2812B 800 kHz protocol requires strict timing:
  - Bit `0`: 350 ns HIGH followed by 800 ns LOW.
  - Bit `1`: 700 ns HIGH followed by 600 ns LOW.
- **Dynamic Register Mapping:** Rather than hardcoding I/O ports in assembly, the driver dynamically resolves the AVR port address and bitmask at runtime (`portOutputRegister(digitalPinToPort(pin_))` and `digitalPinToBitMask(pin_)`), enabling zero-recompilation pin reassignments.
- **Host Testing Support:** In host mode (`-DHOST_TEST`), the driver logs byte buffers cleanly, enabling unit testing without hardware.

---

### 2. Polimorphic UI Abstraction (`StripLedPanel`)

Thanks to the interface inversion introduced in Chapter 6 ([`ILedPanel`](../src/ui/interfaces/i_led_panel.hpp)), migrating the machine from discrete LEDs to the RGB strip required **zero modifications** to `PanelController` or domain logic.

The [`StripLedPanel`](../src/ui/strip_led_panel.hpp) implements:
- **Dual-Tone Modern Aesthetic:** Water levels glow exclusively in Cyan (`0, 220, 255`), wash stages in Pure White (`255, 255, 255`), and the softener indicator in a pastel rose (`255, 100, 140`).
- **Smooth "Breathing" Animation (2-second Period):** The active running stage oscillates smoothly in brightness, future stages maintain a faint 15% standby glow, finished stages extinguish, and paused states pulse synchronously.
- **Integer-Only Wave Math:** Rather than importing heavy floating-point `sin()` math (which consumes ~1.5 KB of AVR Flash), the breathing animation employs integer-only symmetrical triangle wave math:
  ```cpp
  uint16_t phase = now % 2000;
  uint8_t wave = (phase < 1000) ? (phase * 255) / 1000 : ((2000 - phase) * 255) / 1000;
  ```
- **Isolated Diagnostic Strobe:** Inlet timeout (`FILL_TIMEOUT`) blinks water levels in Red; drain pump timeout (`DRAIN_TIMEOUT`) blinks program stages in Red.

---

### 3. Hardware Re-Spin & The Analog "D13 Pull-Up Trap"

Reorganizing the physical ATmega328P wiring reduced the panel interconnect from 12 messy loose wires to a single **7-conductor ribbon cable** with a shared GND:
- `+5V` (Strip VCC)
- `GND` (Common ground for WS2812 strip and all 4 pushbuttons)
- `D6` (WS2812 DIN data line, direct 0Ω connection)
- `D7 / A3` (Softener button)
- `A0 (Pin 14)` (Program button, with 1kΩ series protection)
- `A1 (Pin 15)` (Start-Pause button, with 1kΩ series protection)
- `A2 (Pin 16)` (Water Level button, with 1kΩ series protection)
- **Freed Pins:** Analog pins **`A4 (SDA)`** and **`A5 (SCL)`** were successfully liberated and reserved for the upcoming I2C accelerometer bus.

#### ⚠️ The D13 Electrical Clamping Trap
During initial board testing, buttons on pins A0, A1, and A2 responded flawlessly, but a button wired to **D13** failed to trigger any clicks despite electrical contact.

**The Root Cause:**
- On Arduino Pro Mini boards, D13 has a surface-mount LED and series resistor tied to GND.
- When configuring D13 with `INPUT_PULLUP`, the weak internal pull-up (~30 kΩ) forms a voltage divider with the onboard LED.
- The forward voltage drop ($V_f$) of the red LED clamps the pin at $\approx 1.8\text{V} - 2.0\text{V}$ when the button is released.
- On an ATmega328P powered at 5V, the minimum HIGH input threshold ($V_{IH}$) is $0.6 \times V_{CC} = \mathbf{3.0\text{V}}$.
- Because 1.8V is well below 3.0V, the digital Schmitt trigger never saw a valid release transition (`LEVEL_HIGH`), keeping the button FSM permanently trapped!
- **Resolution:** Buttons were placed on clean GPIOs without onboard LEDs (A0, A1, A2, A3), while the 1kΩ external series resistors reliably pull the pin down to $V_{pin} \approx 0.16\text{V} \ll 1.5\text{V}$ ($V_{IL}$), providing superior ESD and noise rejection.

---

### 4. The AVR Interrupt Collision Dilemma: Tone vs. CLI

When testing long acoustic beeps, an unexpected audio glitch occurred: the piezo buzzer emitted a stuttering, tremolo buzz rather than a smooth, pure tone.

#### The Analysis:
1. The Arduino `tone()` function configures **Timer 2** in CTC mode to toggle the buzzer pin at 3000 Hz, firing an interrupt every **$166\,\mu\text{s}$**.
2. The WS2812B bit-banging protocol demands zero jitter, requiring interrupts to be completely disabled (`cli()`) during transmission.
3. Transmitting 9 pixels ($9 \times 24\text{ bits} \times 1.25\,\mu\text{s}$) holds interrupts disabled for **$270\,\mu\text{s}$**.
4. Because $270\,\mu\text{s} > 166\,\mu\text{s}$, the Timer 2 interrupt was delayed or missed **50 times every second** (at 50 FPS), audibly modulating the carrier wave with a 50 Hz flutter!

#### The Clean Architectural Solution:
Because WS2812 pixels contain internal latching shift registers, **LEDs do not need continuous data transmission to maintain their state**.

Rather than coupling the Buzzer to the Strip, the **`PanelController`** coordinates UI execution:
```cpp
void PanelController::update()
{
    buzzer_.update();

    // Suppress LED frame pushes during active audio output:
    if (!buzzer_.is_playing()) {
        led_panel_.update();
    }
    ...
```
- While a 50 ms button click beep or repeated loud finish beeps are sounding, frame transmission is paused.
- The human eye cannot perceive a 50 ms pause in a slow 2-second breathing wave, but the human ear instantly perceives the pristine, pure 3000 Hz tone.
- In addition, the strip frame rate was throttled to **30 FPS** (`frame_interval_ms = 33`), reducing the total CPU duty cycle consumed by the WS2812 engine to under **0.8%**!

---

### 5. Appliance Standby & Wake-Up UX

To match the behavior of modern domestic appliances:
- **Standby Sleep:** When the cycle completes, the coordinator enters `FINISHED`. The LED strip shuts off completely to save power.
- **Wake-Up on Interaction:** Touching any configuration button (Program, Level, Softener) automatically calls `coordinator.stop_cycle()`, transitioning the machine back to `IDLE` and instantly waking up the LED display with the updated settings.

---

### 6. Milestone Metrics & Readiness

| Metric | Legacy v0.1.0 | FSM v0.3.1 (Discrete) | Strip v0.4.0 (WS2812) |
| :--- | :--- | :--- | :--- |
| **Flash ROM** | 7,130 B (23%) | 14,976 B (48%) | **15,822 B (51%)** |
| **Static SRAM** | 358 B (17%) | 721 B (35%) | **821 B (40%)** |
| **Dynamic Heap** | 0 B | 0 B | **0 B (Zero Heap)** |
| **Automated Tests** | 0 tests | 80 tests | **100 tests (100% pass, 30 ms)** |
| **Panel Wires** | 12 loose wires | 12 loose wires | **7-conductor flat ribbon** |
| **Free GPIOs** | 0 pins | 0 pins | **A4/A5 I2C + A6/A7 Free** |

The firmware is now **production-ready and field-tested** for all domestic washing machines with functional mechanical balance.

---

## Chapter 8: Safety Watchdogs, Out-of-Balance Sensing & I2C Vibration *(Upcoming - v0.4.0)*
*(Coming in v0.4.0: Accelerometer integration on the liberated I2C bus [A4/A5], detecting severe mechanical off-balance during spin acceleration, and AVR hardware watchdogs).*

---

## Chapter 9: Hardware Migration to 32-bit Architecture (ESP32-C3) *(Upcoming)*
*(Coming in future releases: Native FreeRTOS multitasking, telemetry, and migration to modern 32-bit RISC-V silicon).*

