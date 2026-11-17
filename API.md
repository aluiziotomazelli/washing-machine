> **Note**: This document is automatically generated from Doxygen comments in source headers.

# Washing Machine Controller API Reference

---

## Table of Contents

- [Data Types and Structures](#data-types-and-structures)
  - [`ui::ButtonConfig`](#uibuttonconfig)
  - [`fsm::CoordinatorConfig`](#fsmcoordinatorconfig)
  - [`persistence::CycleSnapshot`](#persistencecyclesnapshot)
  - [`ui::DiscreteLedPins`](#uidiscreteledpins)
  - [`hal::LevelSensorPin`](#hallevelsensorpin)
  - [`hal::PressureSwitchConfig`](#halpressureswitchconfig)
  - [`hal::RgbColor`](#halrgbcolor)
  - [`controllers::SpinConfig`](#controllersspinconfig)
  - [`controllers::SprintStep`](#controllerssprintstep)
  - [`ui::StripLedConfig`](#uistripledconfig)
  - [`hal::Vector3`](#halvector3)
  - [`controllers::VibrationConfig`](#controllersvibrationconfig)
  - [`SpinSubPhase`](#spinsubphase)
  - [`WaterLevel`](#waterlevel)
  - [`WashProgram`](#washprogram)
  - [`WashStage`](#washstage)
  - [`MachineState`](#machinestate)
  - [`MachineError`](#machineerror)
  - [`DiagnosticStep`](#diagnosticstep)
  - [`CycleStep`](#cyclestep)
  - [`GpioMode`](#gpiomode)
  - [`GpioLevel`](#gpiolevel)
  - [`MotorState`](#motorstate)
  - [`WatchdogTimeout`](#watchdogtimeout)
  - [`AccelScale`](#accelscale)
  - [`ContactType`](#contacttype)
  - [`ButtonClickType`](#buttonclicktype)
  - [`BuzzerPattern`](#buzzerpattern)
- [Public API (controllers::Agitator)](#public-api-controllersagitator)
- [Public API (hal::ArduinoEepromHAL)](#public-api-halarduinoeepromhal)
- [Public API (hal::ArduinoGpioHAL)](#public-api-halarduinogpiohal)
- [Public API (hal::ArduinoI2cHAL)](#public-api-halarduinoi2chal)
- [Public API (hal::ArduinoTimerHAL)](#public-api-halarduinotimerhal)
- [Public API (hal::ArduinoWatchdogHAL)](#public-api-halarduinowatchdoghal)
- [Public API (ui::Button)](#public-api-uibutton)
- [Public API (ui::Buzzer)](#public-api-uibuzzer)
- [Public API (persistence::CyclePersistenceManager)](#public-api-persistencecyclepersistencemanager)
- [Public API (ui::DiagnosticController)](#public-api-uidiagnosticcontroller)
- [Public API (hal::DigitalOutput)](#public-api-haldigitaloutput)
- [Public API (ui::DiscreteLedPanel)](#public-api-uidiscreteledpanel)
- [Public API (controllers::DrainController)](#public-api-controllersdraincontroller)
- [Public API (controllers::FillController)](#public-api-controllersfillcontroller)
- [Public API (hal::IAccelerometer)](#public-api-haliaccelerometer)
- [Public API (ui::IButton)](#public-api-uiibutton)
- [Public API (ui::IBuzzer)](#public-api-uiibuzzer)
- [Public API (persistence::ICyclePersistence)](#public-api-persistenceicyclepersistence)
- [Public API (hal::IDigitalOutput)](#public-api-halidigitaloutput)
- [Public API (hal::IEepromHAL)](#public-api-halieepromhal)
- [Public API (hal::IGpioHAL)](#public-api-haligpiohal)
- [Public API (hal::II2cHAL)](#public-api-halii2chal)
- [Public API (ui::ILedPanel)](#public-api-uiiledpanel)
- [Public API (hal::IReversibleMotor)](#public-api-halireversiblemotor)
- [Public API (hal::IRgbStrip)](#public-api-halirgbstrip)
- [Public API (hal::ITimerHAL)](#public-api-halitimerhal)
- [Public API (controllers::IVibrationMonitor)](#public-api-controllersivibrationmonitor)
- [Public API (hal::IWatchdogHAL)](#public-api-haliwatchdoghal)
- [Public API (hal::IWaterLevelSensor)](#public-api-haliwaterlevelsensor)
- [Public API (hal::Mpu6050)](#public-api-halmpu6050)
- [Public API (ui::PanelController)](#public-api-uipanelcontroller)
- [Public API (hal::PressureSwitchSensor)](#public-api-halpressureswitchsensor)
- [Public API (hal::ReversibleMotor)](#public-api-halreversiblemotor)
- [Public API (controllers::SpinController)](#public-api-controllersspincontroller)
- [Public API (ui::StripLedPanel)](#public-api-uistripledpanel)
- [Public API (controllers::VibrationMonitor)](#public-api-controllersvibrationmonitor)
- [Public API (fsm::WashCycleCoordinator)](#public-api-fsmwashcyclecoordinator)
- [Public API (hal::Ws2812Strip)](#public-api-halws2812strip)

---

## Data Types and Structures

### `ui::ButtonConfig`

Configuration parameters for electrical behavior and timing thresholds.

| Field | Type | Description |
| :--- | :--- | :--- |
| `active_low` | `bool` | true when the pin reads LOW while pressed. |
| `enable_internal_pull` | `bool` | Enable the MCU internal pull-up resistor. |
| `debounce_press_ms` | `uint16_t` | Contact stabilization time after a press edge. |
| `debounce_release_ms` | `uint16_t` | Contact stabilization time after a release edge. |
| `double_click_ms` | `uint16_t` | Maximum gap between clicks to report DOUBLE_CLICK. |
| `long_click_ms` | `uint16_t` | Hold time that promotes a press to LONG_CLICK. |
| `very_long_click_ms` | `uint16_t` | Hold time that promotes a press to VERY_LONG_CLICK. |
| `timeout_ms` | `uint16_t` | Maximum press hold before reporting TIMEOUT. |

### `fsm::CoordinatorConfig`

Configuration parameters for cycle timings (in seconds or milliseconds).

| Field | Type | Description |
| :--- | :--- | :--- |
| `stage_settle_ms` | `uint32_t` | Quiet delay between major stages (4 s). |
| `normal_wash_agitate_sec` | `uint32_t` | Normal wash continuous agitation (18 min). |
| `heavy_wash_agitate1_sec` | `uint32_t` | Heavy wash gentle agitation (8 min). |
| `heavy_wash_soak_sec` | `uint32_t` | Heavy wash soak (20 min). |
| `heavy_wash_agitate2_sec` | `uint32_t` | Heavy wash normal agitation (14 min). |
| `single_rinse_agitate_sec` | `uint32_t` | Single rinse agitation (7 min). |
| `double_rinse_1_agitate_sec` | `uint32_t` | First rinse agitation (5 min). |
| `double_rinse_2_agitate_sec` | `uint32_t` | Second rinse gentle agitation (2 min). |
| `double_rinse_2_soak_sec` | `uint32_t` | Softener soak (5 min). |
| `double_rinse_2_agitate_post_sec` | `uint32_t` | Post-soak agitation (2 min). |
| `intermediate_spin_sec` | `uint32_t` | Intermediate spin after wash and rinse (2 min). |
| `final_spin_sec` | `uint32_t` | Final spin (4 min). |
| `max_unbalance_recoveries` | `uint8_t` | Hydraulic recovery attempts before latching an error. |
| `unbalance_agitate_sec` | `uint32_t` | Brief agitation to redistribute the load (30 s). |
| `unbalance_fill_level` | `domain::WaterLevel` | Minimum refill level for recovery. |

### `persistence::CycleSnapshot`

Represents the persisted operational state of the washing machine.

| Field | Type | Description |
| :--- | :--- | :--- |
| `seq_num` | `uint16_t` | Monotonic sequence number used to pick the newest record. |
| `program` | `WashProgram` | Wash program being executed. |
| `level` | `WaterLevel` | Selected water level. |
| `step_index` | `uint8_t` | Index of the active recipe step. |
| `in_rinse_subcycle` | `bool` | true while executing the second rinse subcycle. |
| `softener_enabled` | `bool` | true when the softener option is enabled. |
| `is_running` | `bool` | true when a cycle was running at save time. |
| `is_paused` | `bool` | true when the cycle was paused at save time. |
| `checksum` | `uint8_t` | CRC-8 over the preceding fields. |

### `ui::DiscreteLedPins`

Pin mapping for discrete indicator LEDs on the washing machine panel.

Default values correspond to ATmega328P pinout (A0=14, A1=15, A2=16).

| Field | Type | Description |
| :--- | :--- | :--- |
| `power` | `uint8_t` | Power indicator (ledOn). |
| `softener` | `uint8_t` | Softener indicator (ledAmaciante). |
| `wash` | `uint8_t` | Wash stage indicator (ledLavar, A2). |
| `rinse` | `uint8_t` | Rinse stage indicator (ledEnxaguar, A1). |
| `spin` | `uint8_t` | Spin stage indicator (ledCentrifugar, A0). |
| `level_low` | `uint8_t` | Low level indicator (ledNivelB). |
| `level_med` | `uint8_t` | Medium level indicator (ledNivelM). |
| `level_high` | `uint8_t` | Optional high level indicator (255 = not wired). |

### `hal::LevelSensorPin`

Configuration for an individual water level switch contact pin.

| Field | Type | Description |
| :--- | :--- | :--- |
| `pin` | `uint8_t` | GPIO pin number (255 = contact not wired). |
| `contact` | `ContactType` | Electrical contact type of the switch. |

### `hal::PressureSwitchConfig`

Configuration parameters for a multi-contact electromechanical pressure switch.

| Field | Type | Description |
| :--- | :--- | :--- |
| `low` | `LevelSensorPin` | Low level contact (31-32 NC, D10). |
| `medium` | `LevelSensorPin` | Medium level contact (11-13 NO, D11). |
| `high` | `LevelSensorPin` | High level contact (21-23 NO; 255 disables it by default). |
| `debounce_ms` | `uint16_t` | Hydraulic stabilization window. |

### `hal::RgbColor`

Standard 24-bit RGB Color structure.

| Field | Type | Description |
| :--- | :--- | :--- |
| `r` | `uint8_t` | Red component (0..255). |
| `g` | `uint8_t` | Green component (0..255). |
| `b` | `uint8_t` | Blue component (0..255). |

### `controllers::SpinConfig`

Configuration timings for spin cycle phases (in milliseconds).

| Field | Type | Description |
| :--- | :--- | :--- |
| `clutch_engage_ms` | `uint32_t` | Clutch engagement delay in milliseconds. |
| `duty_on_ms` | `uint32_t` | Continuous extraction motor pulse duration. |
| `duty_off_ms` | `uint32_t` | Inertia coast duration between extraction pulses. |
| `coast_down_ms` | `uint32_t` | Drum coast-down time before releasing the pump/brake. |
| `sprints` | `const SprintStep *` | Progressive sprint profile to execute. |
| `sprint_count` | `uint8_t` | Number of sprint entries. |
| `max_unbalance_retries` | `uint8_t` | Maximum dry redistribution retries before latching an error. |

### `controllers::SprintStep`

Represents a single sprint phase with tailored motor run and pump pause times.

| Field | Type | Description |
| :--- | :--- | :--- |
| `on_ms` | `uint16_t` | Motor run time for the sprint pulse. |
| `off_ms` | `uint16_t` | Inertia coast time between sprint pulses. |

### `ui::StripLedConfig`

Configuration parameters for WS2812 strip visual presentation.

| Field | Type | Description |
| :--- | :--- | :--- |
| `max_brightness` | `uint8_t` | Global brightness cap (0..255; 80 is roughly 30%). |
| `idle_brightness` | `uint8_t` | Steady brightness during IDLE selection. |
| `future_brightness` | `uint8_t` | Dim brightness for upcoming stages during RUNNING. |
| `breathe_period_ms` | `uint16_t` | Breathing animation period during RUNNING. |
| `blink_interval_ms` | `uint16_t` | Strobe period during PAUSED and ERROR. |
| `frame_interval_ms` | `uint8_t` | Frame throttle interval (~30 FPS). |

### `hal::Vector3`

3-axis raw sensor reading vector (X, Y, Z).

| Field | Type | Description |
| :--- | :--- | :--- |
| `x` | `int16_t` | Raw X-axis acceleration in LSB. |
| `y` | `int16_t` | Raw Y-axis acceleration in LSB. |
| `z` | `int16_t` | Raw Z-axis acceleration in LSB. |

### `controllers::VibrationConfig`

Calibration parameters for vibration detection based on empirical load tests.

| Field | Type | Description |
| :--- | :--- | :--- |
| `sample_period_ms` | `uint16_t` | Acquisition period (20 ms = 50 Hz). |
| `window_samples` | `uint8_t` | Samples per peak-to-peak window (10 = 200 ms). |
| `motion_threshold` | `uint16_t` | Minimum envelope to report physical movement. Not consumed by production spin logic; kept for tests/API compatibility. |
| `accel_warning_threshold` | `uint16_t` | Envelope that flags unbalance during ACCEL (damping). |
| `trip_threshold` | `uint16_t` | Sustained envelope that triggers a trip. |
| `shock_threshold` | `uint16_t` | Single-sample envelope that triggers an immediate trip. |
| `sustained_trip_duration_ms` | `uint16_t` | Time above trip_threshold before latching a trip (800 ms default). |
| `accel_warning_clear` | `uint16_t` | Hysteresis clear during ACCEL (2000 LSB default). |
| `warning_debounce_ms` | `uint16_t` | Debounce before latch (0 = first 200 ms window). |

### `SpinSubPhase`

Internal non-blocking states of the centrifugal extraction cycle.

| Value | Description |
| :--- | :--- |
| `IDLE` | — |
| `CLUTCH_ENGAGE` | — |
| `SPRINT_ON` | — |
| `SPRINT_OFF` | — |
| `DUTY_RUN_ON` | — |
| `DUTY_RUN_OFF` | — |
| `COAST_DOWN` | — |
| `PAUSE_COASTING` | — |
| `PAUSED` | — |
| `STOP_COASTING` | — |
| `RETRY_COASTING` | — |

### `WaterLevel`

Water level states inside the washing machine tub.

| Value | Description |
| :--- | :--- |
| `EMPTY` | — |
| `LOW_LEVEL` | — |
| `MEDIUM_LEVEL` | — |
| `HIGH_LEVEL` | — |
| `COUNT` | — |

### `WashProgram`

Selectable wash programs.

| Value | Description |
| :--- | :--- |
| `NORMAL_WASH` | — |
| `HEAVY_WASH` | — |
| `RINSE_ONLY` | — |
| `SPIN_ONLY` | — |
| `COUNT` | — |

### `WashStage`

Operational washing stages during cycle execution and visual feedback.

| Value | Description |
| :--- | :--- |
| `IDLE` | — |
| `WASH` | — |
| `RINSE` | — |
| `SPIN` | — |

### `MachineState`

Machine macro operational states.

| Value | Description |
| :--- | :--- |
| `IDLE` | — |
| `RUNNING` | — |
| `PAUSED` | — |
| `ERROR` | — |
| `FINISHED` | — |

### `MachineError`

Machine fault and timeout error classifications.

| Value | Description |
| :--- | :--- |
| `NONE` | — |
| `FILL_TIMEOUT` | — |
| `DRAIN_TIMEOUT` | — |
| `UNBALANCED_LOAD` | — |

### `DiagnosticStep`

Discrete operational steps in Field Service / Diagnostic Mode.

| Value | Description |
| :--- | :--- |
| `LEVEL_SENSOR` | — |
| `VIBRATION_SENSOR` | — |
| `MAIN_VALVE` | — |
| `SOFTENER_VALVE` | — |
| `DRAIN_PUMP` | — |
| `MOTOR_AGITATE` | — |
| `SPIN_TEST` | — |
| `COUNT` | — |

### `CycleStep`

Discrete operational step types within an orchestrated wash cycle.

| Value | Description |
| :--- | :--- |
| `NONE` | — |
| `FILL_MAIN` | — |
| `FILL_SOFTENER` | — |
| `AGITATE_GENTLE` | — |
| `AGITATE_NORMAL` | — |
| `SOAK` | — |
| `DRAIN` | — |
| `SPIN_INTERMEDIATE` | — |
| `SPIN_FINAL` | — |
| `RECOVERY_FILL` | — |
| `RECOVERY_AGITATE` | — |
| `RECOVERY_DRAIN` | — |
| `SETTLE_PAUSE` | — |
| `FINISHED` | — |

### `GpioMode`

Pin direction and pull configuration.

| Value | Description |
| :--- | :--- |
| `MODE_INPUT` | — |
| `MODE_INPUT_PULLUP` | — |
| `MODE_OUTPUT` | — |

### `GpioLevel`

Logic level for digital pins.

| Value | Description |
| :--- | :--- |
| `LEVEL_LOW` | — |
| `LEVEL_HIGH` | — |

### `MotorState`

Operating states of a reversible AC motor.

| Value | Description |
| :--- | :--- |
| `STOPPED` | Both windings de-energized. |
| `RUNNING_CLOCKWISE` | Clockwise winding energized. |
| `RUNNING_COUNTER_CLOCKWISE` | Counter-clockwise winding energized. |
| `DEAD_TIME_WAIT` | Waiting for the reversal dead-time to elapse. |

### `WatchdogTimeout`

Standard hardware watchdog timer timeout periods.

| Value | Description |
| :--- | :--- |
| `TIMEOUT_120MS` | 120 ms timeout. |
| `TIMEOUT_250MS` | 250 ms timeout. |
| `TIMEOUT_500MS` | 500 ms timeout. |
| `TIMEOUT_1S` | 1 s timeout. |
| `TIMEOUT_2S` | 2 s timeout. |
| `TIMEOUT_4S` | 4 s timeout. |
| `TIMEOUT_8S` | 8 s timeout. |

### `AccelScale`

MPU-6050 accelerometer full scale range options.

| Value | Description |
| :--- | :--- |
| `SCALE_2G` | — |
| `SCALE_4G` | — |
| `SCALE_8G` | — |
| `SCALE_16G` | — |

### `ContactType`

Electrical contact type for pressure switch contacts.

| Value | Description |
| :--- | :--- |
| `NORMALLY_OPEN` | — |
| `NORMALLY_CLOSED` | — |

### `ButtonClickType`

Types of button click events detected by the FSM debouncer.

| Value | Description |
| :--- | :--- |
| `NONE_CLICK` | — |
| `CLICK` | — |
| `DOUBLE_CLICK` | — |
| `LONG_CLICK` | — |
| `VERY_LONG_CLICK` | — |
| `TIMEOUT` | — |
| `ERROR_STATE` | — |

### `BuzzerPattern`

Predefined audio feedback patterns for system feedback and alerts.

| Value | Description |
| :--- | :--- |
| `NONE` | — |
| `SHORT_BEEP` | — |
| `DOUBLE_BEEP` | — |
| `LONG_BEEP` | — |
| `CYCLE_FINISHED` | — |
| `ERROR_ALARM` | — |

---

## Public API (controllers::Agitator)

Manages mechanical washing agitation, alternating CW/CCW motor rotation with precise on/off intervals.

### `Agitator()`

**Construct an agitator bound to a reversible motor and timer.**

```cpp
Agitator(hal::ITimerHAL &timer_hal, hal::IReversibleMotor &motor);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `timer_hal` | Timer abstraction used for stroke and duration timing. |
| `motor` | Reversible motor driven by the agitator. |

---

### `start()`

**Start an agitation cycle.**

```cpp
void start(uint32_t duration_sec, uint16_t on_ms=300, uint16_t off_ms=200);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `duration_sec` | Total duration of the agitation phase in seconds. |
| `on_ms` | Pulse duration in milliseconds (e.g. 300 ms). |
| `off_ms` | Pause duration between strokes in milliseconds (e.g. 200 ms or 300 ms). |

---

### `update()`

**Periodic non-blocking update (stroke alternation and duration tracking).**

```cpp
void update();
```

---

### `pause()`

**Suspend agitation and stop the motor.**

```cpp
void pause();
```

---

### `resume()`

**Resume a previously paused agitation cycle.**

```cpp
void resume();
```

---

### `stop()`

**Stop agitation and de-energize the motor.**

```cpp
void stop();
```

---

### `is_active()`

**Check whether an agitation cycle is running.**

```cpp
bool is_active() const;
```

**Returns:**

* `true while active.`

---

### `is_paused()`

**Check whether agitation is suspended.**

```cpp
bool is_paused() const;
```

**Returns:**

* `true while paused.`

---

### `is_finished()`

**Check whether the agitation duration elapsed.**

```cpp
bool is_finished() const;
```

**Returns:**

* `true when the cycle has finished.`

---

## Public API (hal::ArduinoEepromHAL)

Concrete EEPROM implementation using AVR EEPROM library (<EEPROM.h>).

Employs hardware-level update semantics to avoid unnecessary cell wear.

### `ArduinoEepromHAL()`

```cpp
ArduinoEepromHAL()=default;
```

---

### `read_byte()`

**Read a single byte from the specified EEPROM address.**

```cpp
uint8_t read_byte(uint16_t address) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `address` | Zero-based byte offset into EEPROM. |

**Returns:**

* `Byte stored at the given address.`

---

### `write_byte()`

**Write a single byte to the specified EEPROM address.**

Implementations must use update semantics (only write physical cell if value changed) to preserve EEPROM write cycle endurance.

```cpp
void write_byte(uint16_t address, uint8_t value) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `address` | Zero-based byte offset into EEPROM. |
| `value` | Byte value to write. |

---

### `read_block()`

**Read a contiguous block of bytes from EEPROM.**

```cpp
void read_block(uint16_t address, void *dest, size_t length) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `address` | Starting address in EEPROM. |
| `dest` | Pointer to destination memory buffer in RAM. |
| `length` | Number of bytes to read. |

---

### `write_block()`

**Write a contiguous block of bytes to EEPROM with cell-level update semantics.**

```cpp
void write_block(uint16_t address, const void *src, size_t length) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `address` | Starting address in EEPROM. |
| `src` | Pointer to source memory buffer in RAM. |
| `length` | Number of bytes to write. |

---

### `capacity()`

**Get the total usable EEPROM storage capacity in bytes (e.g.**

1024 on ATmega328P).

```cpp
size_t capacity() const override;
```

**Returns:**

* `Total size of EEPROM memory space in bytes.`

---

## Public API (hal::ArduinoGpioHAL)

Concrete GPIO and Tone implementation using standard Arduino core functions.

### `ArduinoGpioHAL()`

```cpp
ArduinoGpioHAL()=default;
```

---

### `set_mode()`

**Configure the operating mode of a pin.**

```cpp
void set_mode(uint8_t pin, GpioMode mode) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |
| `mode` | Desired mode (MODE_INPUT, MODE_INPUT_PULLUP, MODE_OUTPUT). |

---

### `set_level()`

**Set digital output level of a pin.**

```cpp
void set_level(uint8_t pin, GpioLevel level) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |
| `level` | Output level (LEVEL_LOW or LEVEL_HIGH). |

---

### `get_level()`

**Read the digital input level of a pin.**

```cpp
GpioLevel get_level(uint8_t pin) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |

**Returns:**

* `Current logic level (LEVEL_LOW or LEVEL_HIGH).`

---

### `play_tone()`

**Generate a hardware square wave / audio tone on a pin (non-blocking).**

```cpp
void play_tone(uint8_t pin, uint16_t frequency_hz) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |
| `frequency_hz` | Tone frequency in Hertz. |

---

### `stop_tone()`

**Stop hardware audio tone generation on a pin.**

```cpp
void stop_tone(uint8_t pin) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |

---

## Public API (hal::ArduinoI2cHAL)

Concrete I2C implementation wrapping Arduino's Wire library with anti-lockup timeout.

### `ArduinoI2cHAL()`

```cpp
ArduinoI2cHAL()=default;
```

---

### `init()`

**Initialize I2C peripheral and clock speed.**

```cpp
void init(uint32_t clock_hz=100000) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `clock_hz` | Bus clock frequency in Hz (default: 100000 Hz / 100 kHz). |

---

### `write_reg()`

**Write a single byte to a device register.**

```cpp
bool write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `dev_addr` | 7-bit I2C device address. |
| `reg_addr` | Register address inside the device. |
| `value` | Byte value to write. |

**Returns:**

* `true on ACK, false on NACK, bus error or timeout.`

---

### `read_bytes()`

**Read contiguous bytes from a device register.**

```cpp
bool read_bytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buffer, size_t len) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `dev_addr` | 7-bit I2C device address. |
| `reg_addr` | Register address to start reading from. |
| `buffer` | Pointer to destination memory buffer. |
| `len` | Number of bytes to read. |

**Returns:**

* `true on success with full byte count received, false on any communication fault.`

---

## Public API (hal::ArduinoTimerHAL)

Concrete Timer implementation using Arduino core timing functions (millis, micros, delay).

### `ArduinoTimerHAL()`

```cpp
ArduinoTimerHAL()=default;
```

---

### `get_time_ms()`

**Get elapsed time in milliseconds since system startup.**

```cpp
uint32_t get_time_ms() const override;
```

**Returns:**

* `System time in milliseconds.`

---

### `get_time_us()`

**Get elapsed time in microseconds since system startup.**

```cpp
uint64_t get_time_us() const override;
```

**Returns:**

* `System time in microseconds.`

---

### `delay_ms()`

**Busy-wait delay in milliseconds.**

```cpp
void delay_ms(uint32_t ms) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `ms` | Duration in milliseconds to wait. |

---

## Public API (hal::ArduinoWatchdogHAL)

Concrete Watchdog implementation using AVR hardware Watchdog Timer (avr/wdt.h).

### `ArduinoWatchdogHAL()`

```cpp
ArduinoWatchdogHAL()=default;
```

---

### `enable()`

**Enable the hardware watchdog timer with the specified timeout.**

```cpp
void enable(WatchdogTimeout timeout) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `timeout` | Watchdog timeout duration. |

---

### `kick()`

**Reset/refresh the watchdog timer ("kick the dog").**

```cpp
void kick() override;
```

---

### `disable()`

**Disable the hardware watchdog timer.**

```cpp
void disable() override;
```

---

### `was_reset_by_watchdog()`

**Check if the last system reset was triggered by the watchdog timer.**

```cpp
bool was_reset_by_watchdog() const override;
```

**Returns:**

* `true if system recovered from a watchdog reset.`

---

## Public API (ui::Button)

Platform-agnostic button driver with non-blocking FSM debouncing, multi-click and long-press support.

The button states were made based on the library/article: https://www.sigmdel.ca/michel/program/esp8266/arduino/switch_debouncing_en.html by Michel Deslierres.

Modifications: instead of multi-click (double, triple click, etc.), only double-click is used, and a very long click was added to access hidden functions.

### `Button()`

**Construct a button driver bound to a GPIO pin.**

```cpp
Button(hal::IGpioHAL &gpio_hal, hal::ITimerHAL &timer_hal, uint8_t pin, const ButtonConfig &config=ButtonConfig{});
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `gpio_hal` | GPIO abstraction used to sample the pin. |
| `timer_hal` | Timer abstraction used for debounce/click timing. |
| `pin` | Hardware pin connected to the button. |
| `config` | Electrical and timing configuration. |

---

### `init()`

**Initialize hardware pin mode and internal state.**

```cpp
void init() override;
```

---

### `deinit()`

**Deinitialize pin if needed.**

```cpp
void deinit() override;
```

---

### `update()`

**Periodic non-blocking state machine update (to be called in loop).**

```cpp
void update() override;
```

---

### `get_last_click()`

**Consume and return the last detected click event.**

```cpp
ButtonClickType get_last_click() override;
```

**Returns:**

* `ButtonClickType The event detected, resets to NONE_CLICK on read.`

---

### `is_pressed()`

**Check whether the button is currently physically depressed.**

```cpp
bool is_pressed() const override;
```

**Returns:**

* `true if currently pressed, false otherwise.`

---

## Public API (ui::Buzzer)

Platform-agnostic non-blocking buzzer driver supporting passive piezo transducers (tone generation) as well as active buzzers.

### `Buzzer()`

**Construct a buzzer driver bound to a GPIO pin.**

```cpp
Buzzer(hal::IGpioHAL &gpio_hal, hal::ITimerHAL &timer_hal, uint8_t pin, uint16_t frequency_hz=3000);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `gpio_hal` | GPIO/tone abstraction used to drive the buzzer. |
| `timer_hal` | Timer abstraction used for pattern timing. |
| `pin` | Hardware pin connected to the buzzer. |
| `frequency_hz` | Tone frequency for passive piezo transducers (default 3000 Hz). |

---

### `init()`

**Initialize hardware pin mode and ensure buzzer starts silent.**

```cpp
void init() override;
```

---

### `update()`

**Non-blocking state update method (to be called in loop).**

```cpp
void update() override;
```

---

### `beep()`

**Play a single beep with custom duration in milliseconds.**

```cpp
void beep(uint16_t duration_ms=50) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `duration_ms` | Duration of sound in ms (default: 50ms). |

---

### `play_pattern()`

**Play a predefined buzzer alert pattern.**

```cpp
void play_pattern(BuzzerPattern pattern) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pattern` | Desired pattern from BuzzerPattern enum. |

---

### `stop()`

**Immediately silence the buzzer and cancel any ongoing pattern.**

```cpp
void stop() override;
```

---

### `is_playing()`

**Check whether the buzzer is currently sounding or executing a pattern.**

```cpp
bool is_playing() const override;
```

**Returns:**

* `true if active/playing, false if idle/silent.`

---

## Public API (persistence::CyclePersistenceManager)

High-reliability wear-leveling persistence manager using a circular ring buffer.

Distributes EEPROM writes evenly across 64 circular slots without any fixed-address pointer. On boot, a full scan locates the latest valid snapshot using CRC-8 verification and monotonic sequence numbering.

### `CyclePersistenceManager()`

**Construct a persistence manager over an EEPROM HAL.**

```cpp
CyclePersistenceManager(hal::IEepromHAL &eeprom_hal, uint16_t start_address=DEFAULT_START_ADDRESS, uint8_t slot_count=DEFAULT_SLOT_COUNT);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `eeprom_hal` | EEPROM abstraction used for storage. |
| `start_address` | Byte offset of the first ring slot. |
| `slot_count` | Number of circular slots (default 64). |

---

### `load_latest()`

**Load the most recent valid snapshot from non-volatile storage.**

```cpp
bool load_latest(CycleSnapshot &out_snapshot) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `out_snapshot` | Output structure to receive the loaded state. |

**Returns:**

* `true if a valid snapshot was found; false if storage is blank/corrupted.`

---

### `save_user_selection()`

**Persist user program/level preferences during IDLE selection.**

```cpp
void save_user_selection(WashProgram program, WaterLevel level, bool softener_enabled) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Selected wash program. |
| `level` | Selected water level. |
| `softener_enabled` | true when the softener option is enabled. |

---

### `save_cycle_progress()`

**Persist active cycle execution progress across step transitions and pauses.**

```cpp
void save_cycle_progress(WashProgram program, WaterLevel level, bool softener_enabled, uint8_t step_index, bool in_rinse_subcycle, bool is_paused=false) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Active wash program. |
| `level` | Selected water level. |
| `softener_enabled` | true when the softener option is enabled. |
| `step_index` | Index of the current recipe step. |
| `in_rinse_subcycle` | true while executing the second rinse subcycle. |
| `is_paused` | true when the cycle is suspended by the user. |

---

### `save_cycle_finished()`

**Persist cycle completion or user stop (clears running flag).**

```cpp
void save_cycle_finished(WashProgram program, WaterLevel level, bool softener_enabled) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Wash program that was executed. |
| `level` | Water level that was used. |
| `softener_enabled` | true when the softener option was enabled. |

---

### `get_current_seq_num()`

**Get the monotonic sequence number of the last written snapshot.**

```cpp
uint16_t get_current_seq_num() const;
```

**Returns:**

* `Sequence number.`

---

### `get_current_slot()`

**Get the ring-buffer slot index of the last written snapshot.**

```cpp
uint8_t get_current_slot() const;
```

**Returns:**

* `Slot index.`

---

### `has_valid_snapshot()`

**Check whether a valid snapshot was found during the last boot scan.**

```cpp
bool has_valid_snapshot() const;
```

**Returns:**

* `true when recovery data is available.`

---

### `get_cached_snapshot()`

**Get the cached snapshot of the most recent valid record.**

```cpp
const CycleSnapshot & get_cached_snapshot() const;
```

**Returns:**

* `Constant reference to the cached CycleSnapshot.`

---

## Public API (ui::DiagnosticController)

Manages isolated bench testing and hardware diagnostics.

Provides an in-situ self-test routine for technicians and field verification.

### `DiagnosticController()`

**Construct the diagnostic controller with all hardware resources.**

```cpp
DiagnosticController(IButton &btn_start, IButton &btn_program, ILedPanel &led_panel, IBuzzer &buzzer, hal::IWaterLevelSensor &water_sensor, hal::IDigitalOutput &valve_main, hal::IDigitalOutput &valve_softener, hal::IDigitalOutput &drain_pump, hal::IReversibleMotor &motor, hal::ITimerHAL &timer_hal, controllers::IVibrationMonitor *vib_monitor=nullptr);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `btn_start` | Start button used to toggle the current test. |
| `btn_program` | Program button used to advance to the next test. |
| `led_panel` | Panel used to render diagnostic feedback. |
| `buzzer` | Buzzer used for audible feedback. |
| `water_sensor` | Water level sensor under test. |
| `valve_main` | Main inlet valve under test. |
| `valve_softener` | Softener valve under test. |
| `drain_pump` | Drain pump under test. |
| `motor` | Reversible motor under test. |
| `timer_hal` | Timer abstraction used for spin delays. |
| `vib_monitor` | Optional vibration monitor used during the spin test. |

---

### `enter()`

**Enter diagnostic mode at the first test step.**

```cpp
void enter();
```

---

### `exit()`

**Exit diagnostic mode and release all actuators.**

```cpp
void exit();
```

---

### `update()`

**Periodic non-blocking update of the active diagnostic step.**

```cpp
void update();
```

---

### `is_active()`

**Check whether diagnostic mode is currently active.**

```cpp
bool is_active() const;
```

**Returns:**

* `true while diagnostics run.`

---

### `get_current_step()`

**Get the diagnostic step currently being executed.**

```cpp
DiagnosticStep get_current_step() const;
```

**Returns:**

* `Active DiagnosticStep.`

---

### `is_valve_active()`

**Check whether an inlet valve is energized by the test.**

```cpp
bool is_valve_active() const;
```

**Returns:**

* `true when a valve test is active.`

---

### `is_pump_active()`

**Check whether the drain pump is energized by the test.**

```cpp
bool is_pump_active() const;
```

**Returns:**

* `true when the pump test is active.`

---

### `is_agitate_active()`

**Check whether the agitation test is running.**

```cpp
bool is_agitate_active() const;
```

**Returns:**

* `true while the agitator is active.`

---

### `is_spin_active()`

**Check whether the spin test is running.**

```cpp
bool is_spin_active() const;
```

**Returns:**

* `true while the spin test is active.`

---

### `is_spin_tripped()`

**Check whether the spin test tripped on unbalance.**

```cpp
bool is_spin_tripped() const;
```

**Returns:**

* `true when the vibration guard stopped the test.`

---

## Public API (hal::DigitalOutput)

Platform-agnostic digital output controller for relays, valves, and indicators.

Uses an intrusive static linked list to allow zero-overhead, heap-free tracking of all instantiated outputs and global batch operations (such as emergency shut-off via turn_off_all()).

### `DigitalOutput()`

**Construct a new DigitalOutput and register it into the static linked list.**

```cpp
DigitalOutput(IGpioHAL &gpio_hal, uint8_t pin, bool active_low=false, bool initial_state=false);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `gpio_hal` | Reference to the GPIO hardware abstraction layer. |
| `pin` | Hardware pin number. |
| `active_low` | Set true if the relay/load activates on LOW level. |
| `initial_state` | Initial state (true = ON, false = OFF). |

---

### `init()`

**Initialize hardware pin direction and default state.**

```cpp
void init() override;
```

---

### `turn_on()`

**Energize / activate the output device.**

```cpp
void turn_on() override;
```

---

### `turn_off()`

**De-energize / deactivate the output device.**

```cpp
void turn_off() override;
```

---

### `toggle()`

**Invert the current output state.**

```cpp
void toggle() override;
```

---

### `is_on()`

**Query whether the device is currently active.**

```cpp
bool is_on() const override;
```

**Returns:**

* `true if energized/active, false otherwise.`

---

### `get_pin()`

**Get the hardware pin number assigned to this output.**

```cpp
uint8_t get_pin() const override;
```

**Returns:**

* `GPIO pin number.`

---

## Public API (ui::DiscreteLedPanel)

Concrete implementation of ILedPanel driving individual LEDs via GPIO pins.

### `DiscreteLedPanel()`

**Construct a discrete LED panel.**

```cpp
DiscreteLedPanel(hal::IGpioHAL &gpio_hal, hal::ITimerHAL &timer_hal, const DiscreteLedPins &pins=DiscreteLedPins{}, bool active_high=true);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `gpio_hal` | GPIO abstraction used to drive the LEDs. |
| `timer_hal` | Timer abstraction used for blink timing. |
| `pins` | Pin mapping for each indicator LED. |
| `active_high` | true when an LED illuminates on a HIGH level. |

> **Note**: The default level_med pin is D12 (ledNivelM) on the original board, which conflicts with config::k_pressure_switch_high_pin on the current reference build. Use StripLedPanel for that build.

---

### `init()`

**Initialize hardware pins / LED driver.**

```cpp
void init() override;
```

---

### `update()`

**Non-blocking frame / animation update (to be called periodically in loop).**

```cpp
void update() override;
```

---

### `set_machine_state()`

**Update the machine macro state for visual feedback (IDLE, RUNNING, PAUSED, ERROR, FINISHED).**

```cpp
void set_machine_state(domain::MachineState state, domain::MachineError error=domain::MachineError::NONE) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `state` | Current machine macro state. |
| `error` | Active fault associated with ERROR state (NONE when healthy). |

---

### `set_softener()`

**Control the softener mode indicator.**

```cpp
void set_softener(bool enabled) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `enabled` | true to illuminate the softener indicator. |

---

### `set_program()`

**Set the visual program selection during IDLE mode (e.g.**

blinking for heavy wash).

```cpp
void set_program(WashProgram program) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Currently selected wash program. |

---

### `set_stage()`

**Update the wash stage indicator during execution (Wash, Rinse, Spin, or Idle).**

```cpp
void set_stage(WashStage stage) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `stage` | Current wash stage. |

---

### `set_selected_level()`

**Update the selected water level indicator.**

```cpp
void set_selected_level(WaterLevel level) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `level` | Currently selected water level. |

---

### `turn_off_all()`

**Turn off all indicators immediately.**

```cpp
void turn_off_all() override;
```

---

### `show_diagnostic()`

**Render visual feedback for a diagnostic test step.**

```cpp
void show_diagnostic(DiagnosticStep step, uint16_t raw_value=0, bool status_ok=true) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `step` | Current diagnostic test phase. |
| `raw_value` | Auxiliary sensor reading (e.g. current level or vibration amplitude). |
| `status_ok` | Status flag (e.g. true if sensor/I2C is communicating). |

---

## Public API (controllers::DrainController)

Manages tub drainage, controlling the drain pump, bleeding residual water, and enforcing timeout limits.

Supports pause() and resume() to suspend drainage without resetting progress or timeouts.

### `DrainController()`

**Construct a drain controller bound to the drain pump, level sensor and timer.**

```cpp
DrainController(hal::ITimerHAL &timer_hal, hal::IDigitalOutput &drain_pump, hal::IWaterLevelSensor &water_sensor, uint32_t timeout_ms=360000);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `timer_hal` | Timer abstraction used for timeout and bleed accounting. |
| `drain_pump` | Drain pump output. |
| `water_sensor` | Water level sensor used to detect an empty tub. |
| `timeout_ms` | Maximum drain duration before raising an error (default 6 min). |

---

### `start()`

**Start tub drainage.**

```cpp
void start(uint32_t bleed_duration_ms=20000, uint32_t already_empty_duration_ms=0);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `bleed_duration_ms` | Additional pumping time after tub is detected empty (default 20s). |
| `already_empty_duration_ms` | Pumping time if tub is already empty at start (default 0s). |

---

### `update()`

**Periodic non-blocking update (empty detection, bleed and timeout enforcement).**

```cpp
void update();
```

---

### `pause()`

**Suspend drainage and stop the pump while preserving progress.**

```cpp
void pause();
```

---

### `resume()`

**Resume a previously paused drainage operation.**

```cpp
void resume();
```

---

### `stop()`

**Abort drainage immediately and stop the pump.**

```cpp
void stop();
```

---

### `handover()`

**Release the drain output without stopping it (handover to another controller).**

```cpp
void handover();
```

---

### `is_active()`

**Check whether a drain operation is running.**

```cpp
bool is_active() const;
```

**Returns:**

* `true while active.`

---

### `is_paused()`

**Check whether drainage is suspended.**

```cpp
bool is_paused() const;
```

**Returns:**

* `true while paused.`

---

### `is_bleeding()`

**Check whether the post-empty bleed phase is running.**

```cpp
bool is_bleeding() const;
```

**Returns:**

* `true while bleeding residual water.`

---

### `is_finished()`

**Check whether drainage completed.**

```cpp
bool is_finished() const;
```

**Returns:**

* `true when the operation has finished.`

---

### `has_error()`

**Check whether a drain timeout occurred.**

```cpp
bool has_error() const;
```

**Returns:**

* `true when the timeout latched an error.`

---

### `reset_error()`

**Clear a latched drain timeout error.**

```cpp
void reset_error();
```

---

## Public API (controllers::FillController)

Manages water filling sequence, controlling inlet valves and enforcing timeout protection.

Supports pause() and resume() to suspend filling and freeze timeout calculations.

### `FillController()`

**Construct a fill controller bound to valves, level sensor and timer.**

```cpp
FillController(hal::ITimerHAL &timer_hal, hal::IDigitalOutput &valve_main, hal::IDigitalOutput &valve_softener, hal::IWaterLevelSensor &water_sensor, uint32_t timeout_ms=20UL *60 *1000);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `timer_hal` | Timer abstraction used for timeout accounting. |
| `valve_main` | Main inlet valve output. |
| `valve_softener` | Softener dispenser valve output. |
| `water_sensor` | Water level sensor used to detect target level. |
| `timeout_ms` | Maximum fill duration before raising an error (default 20 min). |

---

### `start()`

**Start filling toward a target water level.**

```cpp
void start(domain::WaterLevel target, bool use_softener=false);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `target` | Desired water level. |
| `use_softener` | true to also open the softener valve. |

---

### `update()`

**Periodic non-blocking update (level polling and timeout enforcement).**

```cpp
void update();
```

---

### `pause()`

**Suspend filling and freeze the timeout accounting.**

```cpp
void pause();
```

---

### `resume()`

**Resume a previously paused fill without resetting elapsed time.**

```cpp
void resume();
```

---

### `stop()`

**Abort filling immediately and close the valves.**

```cpp
void stop();
```

---

### `is_active()`

**Check whether a fill operation is currently running.**

```cpp
bool is_active() const;
```

**Returns:**

* `true while filling is active.`

---

### `is_paused()`

**Check whether the fill is suspended.**

```cpp
bool is_paused() const;
```

**Returns:**

* `true while paused.`

---

### `is_finished()`

**Check whether the fill reached its target or was stopped.**

```cpp
bool is_finished() const;
```

**Returns:**

* `true when the operation has finished.`

---

### `has_error()`

**Check whether a fill timeout occurred.**

```cpp
bool has_error() const;
```

**Returns:**

* `true when the timeout latched an error.`

---

### `reset_error()`

**Clear a latched fill timeout error.**

```cpp
void reset_error();
```

---

## Public API (hal::IAccelerometer)

Hardware Abstraction Layer interface for 3-axis accelerometers.

### `init()`

**Initialize and verify communication with the sensor.**

```cpp
bool init();
```

**Returns:**

* `true if sensor responded and configured successfully, false otherwise.`

---

### `read_accel()`

**Read 3-axis acceleration sample.**

```cpp
bool read_accel(Vector3 &accel);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `accel` | Output vector reference to populate on success. |

**Returns:**

* `true if read was completely successful, false on bus error or timeout.`

---

## Public API (ui::IButton)

Abstract interface for button inputs.

### `init()`

**Initialize hardware pin mode and internal state.**

```cpp
void init();
```

---

### `deinit()`

**Deinitialize pin if needed.**

```cpp
void deinit();
```

---

### `update()`

**Periodic non-blocking state machine update (to be called in loop).**

```cpp
void update();
```

---

### `get_last_click()`

**Consume and return the last detected click event.**

```cpp
ButtonClickType get_last_click();
```

**Returns:**

* `ButtonClickType The event detected, resets to NONE_CLICK on read.`

---

### `is_pressed()`

**Check whether the button is currently physically depressed.**

```cpp
bool is_pressed() const;
```

**Returns:**

* `true if currently pressed, false otherwise.`

---

## Public API (ui::IBuzzer)

Abstract interface for non-blocking audible alerts and feedback.

### `init()`

**Initialize hardware pin mode and ensure buzzer starts silent.**

```cpp
void init();
```

---

### `update()`

**Non-blocking state update method (to be called in loop).**

```cpp
void update();
```

---

### `beep()`

**Play a single beep with custom duration in milliseconds.**

```cpp
void beep(uint16_t duration_ms=50);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `duration_ms` | Duration of sound in ms (default: 50ms). |

---

### `play_pattern()`

**Play a predefined buzzer alert pattern.**

```cpp
void play_pattern(BuzzerPattern pattern);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pattern` | Desired pattern from BuzzerPattern enum. |

---

### `stop()`

**Immediately silence the buzzer and cancel any ongoing pattern.**

```cpp
void stop();
```

---

### `is_playing()`

**Check whether the buzzer is currently sounding or executing a pattern.**

```cpp
bool is_playing() const;
```

**Returns:**

* `true if active/playing, false if idle/silent.`

---

## Public API (persistence::ICyclePersistence)

Abstract interface for machine state persistence and power-loss recovery.

### `load_latest()`

**Load the most recent valid snapshot from non-volatile storage.**

```cpp
bool load_latest(CycleSnapshot &out_snapshot);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `out_snapshot` | Output structure to receive the loaded state. |

**Returns:**

* `true if a valid snapshot was found; false if storage is blank/corrupted.`

---

### `save_user_selection()`

**Persist user program/level preferences during IDLE selection.**

```cpp
void save_user_selection(WashProgram program, WaterLevel level, bool softener_enabled);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Selected wash program. |
| `level` | Selected water level. |
| `softener_enabled` | true when the softener option is enabled. |

---

### `save_cycle_progress()`

**Persist active cycle execution progress across step transitions and pauses.**

```cpp
void save_cycle_progress(WashProgram program, WaterLevel level, bool softener_enabled, uint8_t step_index, bool in_rinse_subcycle, bool is_paused=false);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Active wash program. |
| `level` | Selected water level. |
| `softener_enabled` | true when the softener option is enabled. |
| `step_index` | Index of the current recipe step. |
| `in_rinse_subcycle` | true while executing the second rinse subcycle. |
| `is_paused` | true when the cycle is suspended by the user. |

---

### `save_cycle_finished()`

**Persist cycle completion or user stop (clears running flag).**

```cpp
void save_cycle_finished(WashProgram program, WaterLevel level, bool softener_enabled);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Wash program that was executed. |
| `level` | Water level that was used. |
| `softener_enabled` | true when the softener option was enabled. |

---

## Public API (hal::IDigitalOutput)

Abstract interface for binary digital outputs (relays, solenoid valves, pumps, buzzers, LEDs).

### `init()`

**Initialize hardware pin direction and default state.**

```cpp
void init();
```

---

### `turn_on()`

**Energize / activate the output device.**

```cpp
void turn_on();
```

---

### `turn_off()`

**De-energize / deactivate the output device.**

```cpp
void turn_off();
```

---

### `toggle()`

**Invert the current output state.**

```cpp
void toggle();
```

---

### `is_on()`

**Query whether the device is currently active.**

```cpp
bool is_on() const;
```

**Returns:**

* `true if energized/active, false otherwise.`

---

### `get_pin()`

**Get the hardware pin number assigned to this output.**

```cpp
uint8_t get_pin() const;
```

**Returns:**

* `GPIO pin number.`

---

## Public API (hal::IEepromHAL)

Hardware Abstraction Layer interface for non-volatile EEPROM storage.

### `read_byte()`

**Read a single byte from the specified EEPROM address.**

```cpp
uint8_t read_byte(uint16_t address);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `address` | Zero-based byte offset into EEPROM. |

**Returns:**

* `Byte stored at the given address.`

---

### `write_byte()`

**Write a single byte to the specified EEPROM address.**

Implementations must use update semantics (only write physical cell if value changed) to preserve EEPROM write cycle endurance.

```cpp
void write_byte(uint16_t address, uint8_t value);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `address` | Zero-based byte offset into EEPROM. |
| `value` | Byte value to write. |

---

### `read_block()`

**Read a contiguous block of bytes from EEPROM.**

```cpp
void read_block(uint16_t address, void *dest, size_t length);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `address` | Starting address in EEPROM. |
| `dest` | Pointer to destination memory buffer in RAM. |
| `length` | Number of bytes to read. |

---

### `write_block()`

**Write a contiguous block of bytes to EEPROM with cell-level update semantics.**

```cpp
void write_block(uint16_t address, const void *src, size_t length);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `address` | Starting address in EEPROM. |
| `src` | Pointer to source memory buffer in RAM. |
| `length` | Number of bytes to write. |

---

### `capacity()`

**Get the total usable EEPROM storage capacity in bytes (e.g.**

1024 on ATmega328P).

```cpp
size_t capacity() const;
```

**Returns:**

* `Total size of EEPROM memory space in bytes.`

---

## Public API (hal::IGpioHAL)

Hardware Abstraction Layer interface for digital General Purpose I/O and tone generation.

### `set_mode()`

**Configure the operating mode of a pin.**

```cpp
void set_mode(uint8_t pin, GpioMode mode);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |
| `mode` | Desired mode (MODE_INPUT, MODE_INPUT_PULLUP, MODE_OUTPUT). |

---

### `set_level()`

**Set digital output level of a pin.**

```cpp
void set_level(uint8_t pin, GpioLevel level);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |
| `level` | Output level (LEVEL_LOW or LEVEL_HIGH). |

---

### `get_level()`

**Read the digital input level of a pin.**

```cpp
GpioLevel get_level(uint8_t pin);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |

**Returns:**

* `Current logic level (LEVEL_LOW or LEVEL_HIGH).`

---

### `play_tone()`

**Generate a hardware square wave / audio tone on a pin (non-blocking).**

```cpp
void play_tone(uint8_t pin, uint16_t frequency_hz);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |
| `frequency_hz` | Tone frequency in Hertz. |

---

### `stop_tone()`

**Stop hardware audio tone generation on a pin.**

```cpp
void stop_tone(uint8_t pin);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | Hardware pin number. |

---

## Public API (hal::II2cHAL)

Hardware Abstraction Layer interface for I2C (Two-Wire Interface) bus transactions.

### `init()`

**Initialize I2C peripheral and clock speed.**

```cpp
void init(uint32_t clock_hz=100000);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `clock_hz` | Bus clock frequency in Hz (default: 100000 Hz / 100 kHz). |

---

### `write_reg()`

**Write a single byte to a device register.**

```cpp
bool write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `dev_addr` | 7-bit I2C device address. |
| `reg_addr` | Register address inside the device. |
| `value` | Byte value to write. |

**Returns:**

* `true on ACK, false on NACK, bus error or timeout.`

---

### `read_bytes()`

**Read contiguous bytes from a device register.**

```cpp
bool read_bytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buffer, size_t len);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `dev_addr` | 7-bit I2C device address. |
| `reg_addr` | Register address to start reading from. |
| `buffer` | Pointer to destination memory buffer. |
| `len` | Number of bytes to read. |

**Returns:**

* `true on success with full byte count received, false on any communication fault.`

---

## Public API (ui::ILedPanel)

Abstract interface for washing machine visual feedback (supports discrete LEDs or addressable RGB strips).

### `init()`

**Initialize hardware pins / LED driver.**

```cpp
void init();
```

---

### `update()`

**Non-blocking frame / animation update (to be called periodically in loop).**

```cpp
void update();
```

---

### `set_machine_state()`

**Update the machine macro state for visual feedback (IDLE, RUNNING, PAUSED, ERROR, FINISHED).**

```cpp
void set_machine_state(domain::MachineState state, domain::MachineError error=domain::MachineError::NONE);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `state` | Current machine macro state. |
| `error` | Active fault associated with ERROR state (NONE when healthy). |

---

### `set_softener()`

**Control the softener mode indicator.**

```cpp
void set_softener(bool enabled);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `enabled` | true to illuminate the softener indicator. |

---

### `set_program()`

**Set the visual program selection during IDLE mode (e.g.**

blinking for heavy wash).

```cpp
void set_program(WashProgram program);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Currently selected wash program. |

---

### `set_stage()`

**Update the wash stage indicator during execution (Wash, Rinse, Spin, or Idle).**

```cpp
void set_stage(WashStage stage);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `stage` | Current wash stage. |

---

### `set_selected_level()`

**Update the selected water level indicator.**

```cpp
void set_selected_level(WaterLevel level);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `level` | Currently selected water level. |

---

### `turn_off_all()`

**Turn off all indicators immediately.**

```cpp
void turn_off_all();
```

---

### `show_diagnostic()`

**Render visual feedback for a diagnostic test step.**

```cpp
void show_diagnostic(DiagnosticStep step, uint16_t raw_value=0, bool status_ok=true);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `step` | Current diagnostic test phase. |
| `raw_value` | Auxiliary sensor reading (e.g. current level or vibration amplitude). |
| `status_ok` | Status flag (e.g. true if sensor/I2C is communicating). |

---

## Public API (hal::IReversibleMotor)

Abstract interface for reversible AC induction motor control with safety interlocks.

### `init()`

**Initialize hardware pin modes and set safe initial state (stopped).**

```cpp
void init();
```

---

### `update()`

**Non-blocking state update method (to be called periodically in loop).**

```cpp
void update();
```

---

### `rotate_clockwise()`

**Command the motor to rotate clockwise (Right).**

If the motor is currently running counter-clockwise, the implementation must de-energize both windings and honor the configured dead-time before reversing.

```cpp
void rotate_clockwise();
```

---

### `rotate_counter_clockwise()`

**Command the motor to rotate counter-clockwise (Left).**

If the motor is currently running clockwise, the implementation must de-energize both windings and honor the configured dead-time before reversing.

```cpp
void rotate_counter_clockwise();
```

---

### `stop()`

**Immediately de-energize both windings and stop the motor.**

```cpp
void stop();
```

---

### `get_state()`

**Query current operating state of the motor.**

```cpp
MotorState get_state() const;
```

**Returns:**

* `Current MotorState value.`

---

## Public API (hal::IRgbStrip)

Abstract interface for addressable RGB LED strips (e.g.

WS2812B, WS2813, SK6812).

### `init()`

**Initialize hardware pin / peripheral driver.**

```cpp
void init();
```

---

### `set_pixel()`

**Set RGB color of a specific pixel in the buffer.**

```cpp
void set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `index` | Pixel index (0-indexed). |
| `r` | Red component (0..255). |
| `g` | Green component (0..255). |
| `b` | Blue component (0..255). |

---

### `set_pixel()`

**Set RGB color of a specific pixel in the buffer using RgbColor struct.**

```cpp
void set_pixel(uint8_t index, const RgbColor &color);
```

---

### `get_pixel()`

**Get current RGB color of a pixel from the buffer.**

```cpp
RgbColor get_pixel(uint8_t index) const;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `index` | Pixel index (0-indexed). |

**Returns:**

* `Color currently stored for the pixel.`

---

### `clear()`

**Clear all pixels in buffer to (0, 0, 0).**

```cpp
void clear();
```

---

### `show()`

**Push pixel buffer out to physical hardware strip.**

```cpp
void show();
```

---

### `get_num_pixels()`

**Get total number of configured pixels in strip.**

```cpp
uint8_t get_num_pixels() const;
```

**Returns:**

* `Number of addressable pixels.`

---

## Public API (hal::ITimerHAL)

Hardware Abstraction Layer interface for system time and timer services.

### `get_time_ms()`

**Get elapsed time in milliseconds since system startup.**

```cpp
uint32_t get_time_ms() const;
```

**Returns:**

* `System time in milliseconds.`

---

### `get_time_us()`

**Get elapsed time in microseconds since system startup.**

```cpp
uint64_t get_time_us() const;
```

**Returns:**

* `System time in microseconds.`

---

### `delay_ms()`

**Busy-wait delay in milliseconds.**

```cpp
void delay_ms(uint32_t ms);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `ms` | Duration in milliseconds to wait. |

---

## Public API (controllers::IVibrationMonitor)

Interface for mechanical vibration tracking, motion detection, and unbalance protection.

### `init()`

**Initialize or re-initialize monitor state.**

```cpp
void init();
```

---

### `update()`

**Periodic non-blocking processing update.**

Samples accelerometer at configured interval and computes vibration envelope.

```cpp
void update();
```

---

### `get_vibration()`

**Current 3D peak-to-peak vibration envelope metric.**

```cpp
uint16_t get_vibration() const;
```

**Returns:**

* `Vibration index (delta X + delta Y + delta Z over 200 ms window).`

---

### `get_last_sample()`

**Most recent raw 3-axis accelerometer sample.**

```cpp
const hal::Vector3 & get_last_sample() const;
```

**Returns:**

* `Constant reference to the last sampled Vector3 (axis values in raw LSB).`

---

### `is_in_motion()`

**Whether physical movement is detected (agitation or drum spinning).**

```cpp
bool is_in_motion() const;
```

**Returns:**

* `true if vibration exceeds motion threshold.`

---

### `is_warning()`

**Whether vibration is in the warning zone (perceptible unbalance).**

```cpp
bool is_warning() const;
```

**Returns:**

* `true if vibration exceeds warning threshold.`

---

### `is_critical_unbalance()`

**Whether critical unbalance trip condition was reached.**

```cpp
bool is_critical_unbalance() const;
```

**Returns:**

* `true if vibration exceeded the sustained limit (800 ms default) or immediate shock threshold.`

---

### `is_sensor_ok()`

**Whether the physical accelerometer sensor is communicating and healthy.**

```cpp
bool is_sensor_ok() const;
```

**Returns:**

* `true if communication is healthy, false on bus error or disconnected sensor.`

---

### `reset()`

**Reset trip and warning flags, and clear sampling buffer.**

```cpp
void reset();
```

---

## Public API (hal::IWatchdogHAL)

Hardware Abstraction Layer interface for hardware watchdog timer.

### `enable()`

**Enable the hardware watchdog timer with the specified timeout.**

```cpp
void enable(WatchdogTimeout timeout);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `timeout` | Watchdog timeout duration. |

---

### `kick()`

**Reset/refresh the watchdog timer ("kick the dog").**

```cpp
void kick();
```

---

### `disable()`

**Disable the hardware watchdog timer.**

```cpp
void disable();
```

---

### `was_reset_by_watchdog()`

**Check if the last system reset was triggered by the watchdog timer.**

```cpp
bool was_reset_by_watchdog() const;
```

**Returns:**

* `true if system recovered from a watchdog reset.`

---

## Public API (hal::IWaterLevelSensor)

Abstract interface for reading tub water level from mechanical pressure switches or pressure transducers.

### `init()`

**Initialize hardware pins and internal debouncer states.**

```cpp
void init();
```

---

### `update()`

**Periodic non-blocking sensor update (poll inputs and update debouncing).**

```cpp
void update();
```

---

### `is_level_reached()`

**Check whether water has reached or exceeded a target level.**

```cpp
bool is_level_reached(WaterLevel target) const;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `target` | Desired target level (LOW_LEVEL, MEDIUM_LEVEL, HIGH_LEVEL). |

**Returns:**

* `true if target level has been reached, false otherwise.`

---

### `get_current_level()`

**Query the highest water level currently satisfied in the tub.**

```cpp
WaterLevel get_current_level() const;
```

**Returns:**

* `WaterLevel Current measured level.`

---

### `is_empty()`

**Check whether the tub is empty (below the lowest detected level).**

```cpp
bool is_empty() const;
```

**Returns:**

* `true if empty, false if water is present.`

---

## Public API (hal::Mpu6050)

High-efficiency, ultra-lightweight driver for MPU-6050 accelerometer.

Implements IAccelerometer over II2cHAL with zero dynamic allocations, built-in WHO_AM_I device verification, and low-pass filtering.

### `Mpu6050()`

**Construct an MPU-6050 driver bound to an I2C bus.**

```cpp
Mpu6050(II2cHAL &i2c, uint8_t dev_addr=k_default_address, AccelScale scale=AccelScale::SCALE_4G);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `i2c` | I2C HAL used for register transactions. |
| `dev_addr` | 7-bit device address (0x68 by default). |
| `scale` | Accelerometer full-scale range (default +/-4g). |

---

### `init()`

**Initialize and verify communication with the sensor.**

```cpp
bool init() override;
```

**Returns:**

* `true if sensor responded and configured successfully, false otherwise.`

---

### `read_accel()`

**Read 3-axis acceleration sample.**

```cpp
bool read_accel(Vector3 &accel) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `accel` | Output vector reference to populate on success. |

**Returns:**

* `true if read was completely successful, false on bus error or timeout.`

---

### `get_device_address()`

**Get the configured 7-bit I2C device address.**

```cpp
uint8_t get_device_address() const;
```

**Returns:**

* `Device address (0x68 by default).`

---

### `get_scale()`

**Get the configured accelerometer full-scale range.**

```cpp
AccelScale get_scale() const;
```

**Returns:**

* `Active AccelScale setting.`

---

### `is_initialized()`

**Report whether the device was successfully initialized.**

```cpp
bool is_initialized() const;
```

**Returns:**

* `true after a successful WHO_AM_I check and configuration.`

---

## Public API (ui::PanelController)

User Interface controller translating button inputs into coordinator commands and synchronizing LED/buzzer state with the running machine state.

### `PanelController()`

**Construct the UI panel controller with all input/output resources.**

```cpp
PanelController(IButton &btn_start_pause, IButton &btn_program, IButton &btn_water_level, IButton &btn_softener, ILedPanel &led_panel, IBuzzer &buzzer, fsm::WashCycleCoordinator &coordinator, DiagnosticController *diag_ctrl=nullptr, hal::ITimerHAL *timer_hal=nullptr, persistence::ICyclePersistence *persistence=nullptr);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `btn_start_pause` | Start/Pause button input. |
| `btn_program` | Program selection button input. |
| `btn_water_level` | Water level selection button input. |
| `btn_softener` | Softener toggle button input. |
| `led_panel` | Visual feedback panel. |
| `buzzer` | Audible feedback device. |
| `coordinator` | Wash cycle coordinator controlled by the panel. |
| `diag_ctrl` | Optional diagnostic controller; nullptr disables diagnostics. |
| `timer_hal` | Optional timer used for long-press detection. |
| `persistence` | Optional persistence manager used to save user selections. |

---

### `init()`

**Initialize inputs, outputs and restore persisted user preferences.**

```cpp
void init();
```

---

### `update()`

**Periodic non-blocking update (button polling and UI synchronization).**

```cpp
void update();
```

---

### `get_selected_program()`

**Get the currently selected wash program.**

```cpp
domain::WashProgram get_selected_program() const;
```

**Returns:**

* `Selected WashProgram.`

---

### `get_selected_level()`

**Get the currently selected water level.**

```cpp
domain::WaterLevel get_selected_level() const;
```

**Returns:**

* `Selected WaterLevel.`

---

### `is_softener_enabled()`

**Check whether the softener option is enabled.**

```cpp
bool is_softener_enabled() const;
```

**Returns:**

* `true when softener is enabled.`

---

### `is_diagnostic_active()`

**Check whether field-service diagnostic mode is active.**

```cpp
bool is_diagnostic_active() const;
```

**Returns:**

* `true when the diagnostic controller holds control.`

---

## Public API (hal::PressureSwitchSensor)

Platform-agnostic driver for electromechanical washing machine pressure switches.

Supports mixed NC/NO contacts and implements software hydraulic debouncing to filter water sloshing waves during filling and agitation.

### `PressureSwitchSensor()`

**Construct a pressure switch reader for three level contacts.**

```cpp
PressureSwitchSensor(IGpioHAL &gpio_hal, ITimerHAL &timer_hal, const PressureSwitchConfig &config=PressureSwitchConfig{});
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `gpio_hal` | GPIO abstraction used to sample the contact pins. |
| `timer_hal` | Timer abstraction used for hydraulic debouncing. |
| `config` | Pin/contact mapping and debounce window (defaults to the reference board map). |

> **Note**: On the ATmega328P reference build the high-level contact is wired to D12 through config::k_pressure_switch_high_pin. The default Pin255 value disables the high contact when no pin is supplied.

---

### `init()`

**Initialize hardware pins and internal debouncer states.**

```cpp
void init() override;
```

---

### `update()`

**Periodic non-blocking sensor update (poll inputs and update debouncing).**

```cpp
void update() override;
```

---

### `is_level_reached()`

**Check whether water has reached or exceeded a target level.**

```cpp
bool is_level_reached(WaterLevel target) const override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `target` | Desired target level (LOW_LEVEL, MEDIUM_LEVEL, HIGH_LEVEL). |

**Returns:**

* `true if target level has been reached, false otherwise.`

---

### `get_current_level()`

**Query the highest water level currently satisfied in the tub.**

```cpp
WaterLevel get_current_level() const override;
```

**Returns:**

* `WaterLevel Current measured level.`

---

### `is_empty()`

**Check whether the tub is empty (below the lowest detected level).**

```cpp
bool is_empty() const override;
```

**Returns:**

* `true if empty, false if water is present.`

---

## Public API (hal::ReversibleMotor)

Platform-agnostic reversible AC motor driver with hardware/software mutual exclusion and non-blocking dead-time delay management.

### `ReversibleMotor()`

**Construct a reversible motor driver bound to a pair of direction pins.**

```cpp
ReversibleMotor(IGpioHAL &gpio_hal, ITimerHAL &timer_hal, uint8_t pin_clockwise, uint8_t pin_counter_clockwise, uint16_t dead_time_ms=200, bool active_low=false);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `gpio_hal` | GPIO abstraction used to drive the direction pins. |
| `timer_hal` | Timer abstraction used to measure the dead-time window. |
| `pin_clockwise` | GPIO pin that energizes the clockwise winding. |
| `pin_counter_clockwise` | GPIO pin that energizes the counter-clockwise winding. |
| `dead_time_ms` | Minimum idle time between direction reversals (default 200 ms). |
| `active_low` | true when the relay/TRIAC driver is active on a LOW level. |

---

### `init()`

**Initialize hardware pin modes and set safe initial state (stopped).**

```cpp
void init() override;
```

---

### `update()`

**Non-blocking state update method (to be called periodically in loop).**

```cpp
void update() override;
```

---

### `rotate_clockwise()`

**Command the motor to rotate clockwise (Right).**

If the motor is currently running counter-clockwise, the implementation must de-energize both windings and honor the configured dead-time before reversing.

```cpp
void rotate_clockwise() override;
```

---

### `rotate_counter_clockwise()`

**Command the motor to rotate counter-clockwise (Left).**

If the motor is currently running clockwise, the implementation must de-energize both windings and honor the configured dead-time before reversing.

```cpp
void rotate_counter_clockwise() override;
```

---

### `stop()`

**Immediately de-energize both windings and stop the motor.**

```cpp
void stop() override;
```

---

### `get_state()`

**Query current operating state of the motor.**

```cpp
MotorState get_state() const override;
```

**Returns:**

* `Current MotorState value.`

---

## Public API (controllers::SpinController)

Manages centrifugal extraction sequence: clutch engage -> dynamic sprints -> 4s/4s inertia duty cycle -> coast down.

Features mechanical transmission protection: routes pause() and stop() through non-blocking coast-down, keeping the pump/actuator engaged until the drum slows down to avoid violent mechanical braking.

### `SpinController()`

**Construct a spin controller bound to timer, pump, motor and optional monitor.**

```cpp
SpinController(hal::ITimerHAL &timer_hal, hal::IDigitalOutput &drain_pump, hal::IReversibleMotor &motor, const SpinConfig &config=SpinConfig{}, IVibrationMonitor *vibration_monitor=nullptr);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `timer_hal` | Timer abstraction used for phase timing. |
| `drain_pump` | Drain pump / brake clutch output kept engaged during coast-down. |
| `motor` | Reversible motor used for centrifugal extraction. |
| `config` | Sprint profile and phase timings. |
| `vibration_monitor` | Optional unbalance monitor; nullptr disables the guard. |

---

### `start()`

**Start centrifugal spin cycle.**

```cpp
void start(domain::WaterLevel level, uint32_t duration_sec=240);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `level` | Water level selected (determines sprint inertia profile). |
| `duration_sec` | Total duty-run spin duration in seconds (default 4 min). |

---

### `update()`

**Periodic non-blocking update (sub-phase progression and unbalance checks).**

```cpp
void update();
```

---

### `pause()`

**Soft pause: cuts motor immediately but keeps pump energized for coast-down to protect transmission.**

```cpp
void pause();
```

---

### `resume()`

**Resume spinning: re-engages clutch and continues the cycle.**

```cpp
void resume();
```

---

### `stop()`

**Soft stop: cuts motor immediately and coasts down before releasing pump/brake.**

```cpp
void stop();
```

---

### `emergency_stop()`

**Immediate emergency cut without coast-down.**

```cpp
void emergency_stop();
```

---

### `reset_error()`

**Clear latched error and reset unbalance retry counter.**

```cpp
void reset_error();
```

---

### `is_active()`

**Check whether the spin cycle is running.**

```cpp
bool is_active() const;
```

**Returns:**

* `true while active.`

---

### `is_paused()`

**Check whether the spin cycle is suspended.**

```cpp
bool is_paused() const;
```

**Returns:**

* `true while paused.`

---

### `is_finished()`

**Check whether the spin cycle completed.**

```cpp
bool is_finished() const;
```

**Returns:**

* `true when finished.`

---

### `has_error()`

**Check whether an unbalance error is latched.**

```cpp
bool has_error() const;
```

**Returns:**

* `true when the cycle stopped due to unbalance.`

---

### `get_unbalance_retries()`

**Get the number of unbalance redistribution retries already used.**

```cpp
uint8_t get_unbalance_retries() const;
```

**Returns:**

* `Retry count.`

---

### `get_sub_phase()`

**Get the current internal sub-phase.**

```cpp
SpinSubPhase get_sub_phase() const;
```

**Returns:**

* `Active SpinSubPhase.`

---

## Public API (ui::StripLedPanel)

9-Pixel Addressable RGB LED Panel Driver implementing ui::ILedPanel.

Pixel Mapping: [P0]: Spin Stage (Warm white) [P1]: Rinse Stage (Warm white) [P2]: Wash Stage (Warm white) [P3]: Heavy Wash / Soak Indicator (Warm white) [P4]: Gap / Separator (OFF) [P5]: High Level (Cyan) [P6]: Medium Level (Cyan) [P7]: Low Level (Cyan)

### `StripLedPanel()`

**Construct a strip LED panel.**

```cpp
StripLedPanel(hal::IRgbStrip &strip, hal::ITimerHAL &timer_hal, const StripLedConfig &config=StripLedConfig{});
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `strip` | Addressable RGB strip driver. |
| `timer_hal` | Timer abstraction used for animation timing. |
| `config` | Visual animation configuration. |

---

### `init()`

**Initialize hardware pins / LED driver.**

```cpp
void init() override;
```

---

### `update()`

**Non-blocking frame / animation update (to be called periodically in loop).**

```cpp
void update() override;
```

---

### `set_machine_state()`

**Update the machine macro state for visual feedback (IDLE, RUNNING, PAUSED, ERROR, FINISHED).**

```cpp
void set_machine_state(domain::MachineState state, domain::MachineError error=domain::MachineError::NONE) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `state` | Current machine macro state. |
| `error` | Active fault associated with ERROR state (NONE when healthy). |

---

### `set_softener()`

**Control the softener mode indicator.**

```cpp
void set_softener(bool enabled) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `enabled` | true to illuminate the softener indicator. |

---

### `set_program()`

**Set the visual program selection during IDLE mode (e.g.**

blinking for heavy wash).

```cpp
void set_program(WashProgram program) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Currently selected wash program. |

---

### `set_stage()`

**Update the wash stage indicator during execution (Wash, Rinse, Spin, or Idle).**

```cpp
void set_stage(WashStage stage) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `stage` | Current wash stage. |

---

### `set_selected_level()`

**Update the selected water level indicator.**

```cpp
void set_selected_level(WaterLevel level) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `level` | Currently selected water level. |

---

### `turn_off_all()`

**Turn off all indicators immediately.**

```cpp
void turn_off_all() override;
```

---

### `show_diagnostic()`

**Render visual feedback for a diagnostic test step.**

```cpp
void show_diagnostic(DiagnosticStep step, uint16_t raw_value=0, bool status_ok=true) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `step` | Current diagnostic test phase. |
| `raw_value` | Auxiliary sensor reading (e.g. current level or vibration amplitude). |
| `status_ok` | Status flag (e.g. true if sensor/I2C is communicating). |

---

### `get_config()`

**Get the immutable visual configuration in use.**

```cpp
const StripLedConfig & get_config() const;
```

**Returns:**

* `Reference to the active StripLedConfig.`

---

## Public API (controllers::VibrationMonitor)

Autonomous vibration analysis engine computing 3D peak-to-peak envelopes.

### `VibrationMonitor()`

**Construct a vibration monitor bound to an accelerometer and timer.**

```cpp
VibrationMonitor(hal::IAccelerometer &accelerometer, hal::ITimerHAL &timer_hal, const VibrationConfig &config=VibrationConfig{});
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `accelerometer` | Accelerometer source sampled by the monitor. |
| `timer_hal` | Timer abstraction used for sampling and trip timing. |
| `config` | Calibration thresholds and sampling window. |

---

### `init()`

**Initialize or re-initialize monitor state.**

```cpp
void init() override;
```

---

### `update()`

**Periodic non-blocking processing update.**

Samples accelerometer at configured interval and computes vibration envelope.

```cpp
void update() override;
```

---

### `get_vibration()`

**Current 3D peak-to-peak vibration envelope metric.**

```cpp
uint16_t get_vibration() const override;
```

**Returns:**

* `Vibration index (delta X + delta Y + delta Z over 200 ms window).`

---

### `get_last_sample()`

**Most recent raw 3-axis accelerometer sample.**

```cpp
const hal::Vector3 & get_last_sample() const override;
```

**Returns:**

* `Constant reference to the last sampled Vector3 (axis values in raw LSB).`

---

### `is_in_motion()`

**Whether physical movement is detected (agitation or drum spinning).**

```cpp
bool is_in_motion() const override;
```

**Returns:**

* `true if vibration exceeds motion threshold.`

---

### `is_warning()`

**Whether vibration is in the warning zone (perceptible unbalance).**

```cpp
bool is_warning() const override;
```

**Returns:**

* `true if vibration exceeds warning threshold.`

---

### `is_critical_unbalance()`

**Whether critical unbalance trip condition was reached.**

```cpp
bool is_critical_unbalance() const override;
```

**Returns:**

* `true if vibration exceeded the sustained limit (800 ms default) or immediate shock threshold.`

---

### `is_sensor_ok()`

**Whether the physical accelerometer sensor is communicating and healthy.**

```cpp
bool is_sensor_ok() const override;
```

**Returns:**

* `true if communication is healthy, false on bus error or disconnected sensor.`

---

### `reset()`

**Reset trip and warning flags, and clear sampling buffer.**

```cpp
void reset() override;
```

---

### `set_enabled()`

**Enable or disable sample acquisition (disabled monitor keeps last state).**

```cpp
void set_enabled(bool enabled);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `enabled` | true to resume sampling. |

---

### `is_enabled()`

**Check whether sampling is currently enabled.**

```cpp
bool is_enabled() const;
```

**Returns:**

* `true when the monitor is sampling.`

---

## Public API (fsm::WashCycleCoordinator)

Central Finite State Machine (FSM) coordinator orchestrating laundry cycle recipes.

Adheres strictly to the Single Responsibility Principle: does not manipulate pins or relays. Instead, it delegates physical execution to dedicated atomic process controllers (FillController, Agitator, DrainController, SpinController).

### `WashCycleCoordinator()`

**Construct the cycle coordinator with its atomic controllers.**

```cpp
WashCycleCoordinator(hal::ITimerHAL &timer_hal, controllers::FillController &fill_ctrl, controllers::Agitator &agitator, controllers::DrainController &drain_ctrl, controllers::SpinController &spin_ctrl, const CoordinatorConfig &config=CoordinatorConfig{}, persistence::ICyclePersistence *persistence=nullptr);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `timer_hal` | Timer abstraction used for soak/settle timers. |
| `fill_ctrl` | Fill controller used during fill steps. |
| `agitator` | Agitator used during wash/rinse steps. |
| `drain_ctrl` | Drain controller used during drain steps. |
| `spin_ctrl` | Spin controller used during spin steps. |
| `config` | Recipe durations and recovery parameters. |
| `persistence` | Optional persistence manager for power-loss recovery. |

---

### `init()`

**Initialize FSM state and subordinate controllers.**

```cpp
void init();
```

---

### `update()`

**Periodic non-blocking FSM update (step progression and recovery).**

```cpp
void update();
```

---

### `start_cycle()`

**Start a new wash cycle from IDLE.**

```cpp
void start_cycle(WashProgram program, WaterLevel level, bool softener_enabled);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `program` | Selected wash program. |
| `level` | Selected water level. |
| `softener_enabled` | true to enable the softener dispenser step. |

---

### `resume_interrupted_cycle()`

**Resume a cycle from a persisted snapshot after power loss.**

```cpp
void resume_interrupted_cycle(const persistence::CycleSnapshot &snapshot);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `snapshot` | Snapshot loaded from non-volatile storage. |

---

### `pause_cycle()`

**Pause the running cycle and suspend all active controllers.**

```cpp
void pause_cycle();
```

---

### `resume_cycle()`

**Resume a paused cycle.**

```cpp
void resume_cycle();
```

---

### `advance_step()`

**Skip the current step and advance to the next recipe step.**

```cpp
void advance_step();
```

---

### `stop_cycle()`

**Stop the running cycle and return to IDLE.**

```cpp
void stop_cycle();
```

---

### `get_state()`

**Get the current machine macro state.**

```cpp
MachineState get_state() const;
```

**Returns:**

* `Active MachineState.`

---

### `get_error()`

**Get the current latched machine error.**

```cpp
MachineError get_error() const;
```

**Returns:**

* `Active MachineError (NONE when healthy).`

---

### `get_current_stage()`

**Get the current wash stage used for visual feedback.**

```cpp
WashStage get_current_stage() const;
```

**Returns:**

* `Active WashStage.`

---

### `get_current_step()`

**Get the current recipe step.**

```cpp
CycleStep get_current_step() const;
```

**Returns:**

* `Active CycleStep.`

---

### `get_program()`

**Get the active wash program.**

```cpp
WashProgram get_program() const;
```

**Returns:**

* `Active WashProgram.`

---

### `get_original_program()`

**Get the program chosen by the user before any diagnostic override.**

```cpp
WashProgram get_original_program() const;
```

**Returns:**

* `Original WashProgram.`

---

### `get_level()`

**Get the selected water level.**

```cpp
WaterLevel get_level() const;
```

**Returns:**

* `Selected WaterLevel.`

---

### `is_softener_enabled()`

**Check whether the softener option is enabled.**

```cpp
bool is_softener_enabled() const;
```

**Returns:**

* `true when softener dispensing is enabled.`

---

### `get_step_index()`

**Get the index of the current recipe step.**

```cpp
uint8_t get_step_index() const;
```

**Returns:**

* `Zero-based step index.`

---

### `get_unbalance_recoveries()`

**Get the number of unbalance recoveries already attempted.**

```cpp
uint8_t get_unbalance_recoveries() const;
```

**Returns:**

* `Recovery attempt count.`

---

### `set_persistence()`

**Attach or detach the persistence manager at runtime.**

```cpp
void set_persistence(persistence::ICyclePersistence *persistence);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `persistence` | Persistence implementation, or nullptr to disable persistence. |

---

## Public API (hal::Ws2812Strip)

Zero-heap, cycle-accurate WS2812B (NeoPixel) LED strip driver for ATmega328P and Host PC testing.

### `Ws2812Strip()`

**Construct a WS2812 strip driver.**

```cpp
Ws2812Strip(uint8_t pin=6, uint8_t num_pixels=9);
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `pin` | GPIO data pin connected to the strip DIN line (default D6). |
| `num_pixels` | Number of addressable pixels (default 9, capped at k_max_pixels). |

---

### `init()`

**Initialize hardware pin / peripheral driver.**

```cpp
void init() override;
```

---

### `set_pixel()`

**Set RGB color of a specific pixel in the buffer.**

```cpp
void set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `index` | Pixel index (0-indexed). |
| `r` | Red component (0..255). |
| `g` | Green component (0..255). |
| `b` | Blue component (0..255). |

---

### `set_pixel()`

**Set RGB color of a specific pixel in the buffer using RgbColor struct.**

```cpp
void set_pixel(uint8_t index, const RgbColor &color) override;
```

---

### `get_pixel()`

**Get current RGB color of a pixel from the buffer.**

```cpp
RgbColor get_pixel(uint8_t index) const override;
```

**Parameters:**

| Parameter | Description |
| :--- | :--- |
| `index` | Pixel index (0-indexed). |

**Returns:**

* `Color currently stored for the pixel.`

---

### `clear()`

**Clear all pixels in buffer to (0, 0, 0).**

```cpp
void clear() override;
```

---

### `show()`

**Push pixel buffer out to physical hardware strip.**

```cpp
void show() override;
```

---

### `get_num_pixels()`

**Get total number of configured pixels in strip.**

```cpp
uint8_t get_num_pixels() const override;
```

**Returns:**

* `Number of addressable pixels.`

---

### `get_show_count()`

**Number of show() calls issued since boot (host-test inspection helper).**

```cpp
uint32_t get_show_count() const;
```

**Returns:**

* `Monotonic frame counter.`

---
