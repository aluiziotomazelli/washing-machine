#Refactor Plan : Sensor - Driven ACCEL Spin with Adaptive Vibration Damping

## 1. Status and Decision

**Decision:** Replace the blind sprint schedule entirely with a single `ACCEL` phase governed by sensor-driven vibration damping.

**Target architecture:** Option B.

**Removed concepts:**

- `SprintStep`
- `k_default_sprints`
- `SpinConfig::sprints`
- `SpinConfig::sprint_count`
- `SpinSubPhase::SPRINT_ON`
- `SpinSubPhase::SPRINT_OFF`
- `current_sprint_idx_`
- `total_sprints_`
- `current_sprint_on_ms_`
- `current_sprint_off_ms_`
- All sprint-related branches in `start()`, `update()`, `resume()`, and `RETRY_COASTING`
- The `step_through_sprints()` test helper and sprint-based test fixtures

No sprint fallback remains in production code. If a rollback is required, use a Git revert or a dedicated branch, not a hidden runtime flag.

**Submodule impact:** none. This workspace has no Git submodules and no shared HAL dependency that requires a coordinated change.

---

## 2. Problem Statement

The current spin cycle accelerates the drum through a mechanical resonance band using four timed ON/OFF sprint pulses. The sprint profile is open-loop:

- It was calibrated against a limited set of loads and test runs.
- The correct OFF duration depends on the load mass, distribution, water retention, and balance-ring behavior.
- A pause that is too short does not allow enough redistribution.
- A pause that is too long lets the drum coast back into the 200–250 RPM natural-frequency region. Re-energizing the motor there produces a severe dynamic jolt.
- The profile cannot adapt to a different load on the next wash.

The vibration monitor already measures the relevant symptom. The refactor closes the loop: the motor runs continuously during acceleration and is paused only when vibration exceeds a safe limit.

### 2.1 Resonance characterization

`docs/report_rpm_resonance.md` measured the drum ramp from video cuts:

- Resonance onset: approximately **200 RPM**
- Resonance peak: approximately **285–300 RPM**
- Resonance end: approximately **379 RPM**
- Full resonance band: **200–380 RPM**
- Cruise speed: approximately **750–768 RPM**

The report is useful as validation and documentation, not as a runtime control input. The machine has no tachometer, so the firmware cannot use RPM thresholds directly. The vibration sensor measures amplitude, not rotational speed. Attempting to derive RPM from the cabinet accelerometer would be fragile and unnecessary.

The report supports the refactor in two ways:

1. It confirms that the dangerous region is crossed during acceleration, not during stable cruise.
2. It confirms that a continuous ramp can cross the resonance band quickly when the motor is allowed to accelerate without arbitrary OFF timers.

The report does not replace vibration telemetry for threshold calibration.

---

## 3. Goals and Non-Goals

### 3.1 Goals

- Remove the open-loop sprint schedule completely.
- Add a single `SpinSubPhase::ACCEL` state.
- Add `SpinSubPhase::VIBE_DAMPING` as the only adaptive pause mechanism during acceleration and motor-on cruise.
- Preserve the existing critical-unbalance trip and retry behavior.
- Preserve non-blocking operation, zero heap allocation, and AVR-compatible integer arithmetic.
- Keep the vibration decision logic testable on the host without HAL dependencies.
- Improve recovery behavior with longer dry retries, longer hydraulic agitation, and a longer coast-down window.
- Add fail-safes for sensor failure and excessive damping duration.

### 3.2 Non-goals

- Adding an RPM sensor or tachometer.
- Using the accelerometer to estimate RPM.
- Replacing the existing MPU-6050 or its 50 Hz sampling strategy.
- Changing the wash program recipe structure.
- Changing the `WashCycleCoordinator` public API.
- Changing the diagnostic spin test in `DiagnosticController`.
- Introducing dynamic memory, exceptions, or RTOS primitives.

---

## 4. Target Architecture

### 4.1 State machine

```text
IDLE
  |
  v
CLUTCH_ENGAGE
  |
  v
ACCEL  <--------------------------------------+
  |  warning + motor energized                |
  v                                           |
VIBE_DAMPING -- warning clear + min off ------+
  |
  | critical / max off / sensor timeout
  v
RETRY_COASTING or STOP_COASTING

ACCEL -- accumulated motor-on target reached --> DUTY_RUN_ON
DUTY_RUN_ON <--> DUTY_RUN_OFF
DUTY_RUN_ON -- warning + motor energized --> VIBE_DAMPING
DUTY_RUN_OFF -- warning --> no damping action
DUTY_RUN_ON/OFF -- duration reached --> COAST_DOWN
```

### 4.2 Component responsibilities

| Component | File | Responsibility |
|---|---|---|
| `VibrationMonitor` | `src/controllers/vibration_monitor.hpp/.cpp` | Sample accelerometer, compute 3D peak-to-peak envelope, apply warning hysteresis, latch critical trips, report sensor health |
| `IVibrationMonitor` | `src/controllers/interfaces/i_vibration_monitor.hpp` | Existing interface; no signature changes required |
| `VibrationDampingPolicy` | `src/controllers/vibration_damping_policy.hpp/.cpp` | Pure decision logic: run, pause, resume, or escalate |
| `AccelTimeline` | `src/controllers/accel_timeline.hpp/.cpp` | Accumulate motor-on time and enforce the wall-clock safety cap for `ACCEL` |
| `SpinController` | `src/controllers/spin_controller.hpp/.cpp` | Orchestrate clutch, ACCEL, damping, duty run, coast-down, pause, stop, and retry |
| `MockVibrationMonitor` | `test/mocks/mock_vibration_monitor.hpp` | Existing mock;
extend tests as needed | | `WashCycleCoordinator` | `src / fsm / wash_cycle_coordinator.hpp /.cpp` | Unchanged API; retry and recovery defaults updated |

### 4.3 Dependency rules

- `VibrationDampingPolicy` and `AccelTimeline` must not include HAL headers.
- `SpinController` remains the only class that commands the motor and drain pump.
- `VibrationMonitor` remains the only class that reads the accelerometer.
- All time values use `uint32_t` milliseconds from `ITimerHAL`.
- No `static_assert` is used for runtime configuration invariants.

### 4.4 Class diagram

```mermaid
classDiagram
    direction TB

    class SpinController
{
    + start(WaterLevel, uint32_t) + update() + pause() + resume() + stop() + emergency_stop() + reset_error()
}

class VibrationDampingPolicy
{
    + evaluate(DampingInput, bool motor_on, bool damping_active)DampingAction
        + can_resume(DampingInput, uint32_t damping_elapsed_ms)bool
}

class AccelTimeline
{
    + start(uint32_t now_ms) +
        on_motor_on(uint32_t now_ms) + on_motor_off(uint32_t now_ms) + is_complete(uint32_t now_ms)bool +
        motor_on_elapsed_ms()uint32_t
}

class IVibrationMonitor
{
    << interface >> +update() + is_warning() bool + is_critical_unbalance() bool + is_sensor_ok() bool + reset()
}

class VibrationMonitor
{
    + update() + is_warning()bool + is_critical_unbalance()bool + is_sensor_ok()bool
}

class ITimerHAL
{
    << interface >>
} class IReversibleMotor
{
    << interface >>
} class IDigitalOutput
{
    << interface >>
} class IAccelerometer
{
    << interface >>
}

        SpinController-- > IVibrationMonitor SpinController-- > ITimerHAL SpinController-- >
        IReversibleMotor SpinController-- > IDigitalOutput SpinController-- > VibrationDampingPolicy SpinController-- >
        AccelTimeline

            VibrationMonitor..|
    > IVibrationMonitor VibrationMonitor-- > IAccelerometer VibrationMonitor-- >
        ITimerHAL
```

        -- -

        ##5. Configuration Changes

        ## #5.1 `SpinConfig`

        Replace sprint fields with ACCEL and damping parameters.

```cpp struct SpinConfig
{
    uint32_t clutch_engage_ms{5000};
    uint32_t duty_on_ms{4000};
    uint32_t duty_off_ms{4000};
    uint32_t coast_down_ms{18000};
    uint32_t accel_motor_on_ms{7000};
    uint32_t accel_max_wall_ms{40000};
    uint32_t damping_min_off_ms{800};
    uint32_t damping_max_off_ms{8000};
    uint8_t max_unbalance_retries{2};
};
```

    Parameter rationale :

    |
    Parameter |
    Default |
    Rationale |
    |
    -- -|
    -- - : |
           -- -|
           | `clutch_engage_ms` |
           5000 |
           Existing mechanical brake release delay;
unchanged | | `duty_on_ms` | 4000 | Existing cruise pulse;
unchanged | | `duty_off_ms` | 4000 | Existing cruise coast; unchanged |
| `coast_down_ms` | **18000** | 10 s is insufficient for a full stop from cruise speed; see section 7.3 |
| `accel_motor_on_ms` | 7000 | Empirical target for accumulated motor-on acceleration time; calibrate with real loads |
| `accel_max_wall_ms` | 40000 | Safety cap so repeated damping cannot hold the cycle in ACCEL indefinitely |
| `damping_min_off_ms` | 800 | Prevent immediate re-engagement after a warning clears |
| `damping_max_off_ms` | 8000 | Escalate instead of waiting indefinitely for vibration to clear |
| `max_unbalance_retries` | **2** | More dry recovery opportunities; see section 7.1 |

`accel_motor_on_ms` and `accel_max_wall_ms` are empirical calibration values. The RPM report suggests a continuous ramp can cross the resonance band in a few seconds under light load. Seven seconds of motor-on time avoids the end-of-ramp resonance spike observed with two dry jeans while still reaching the duty-run handover.

> `SpinConfig` is currently aggregate-initialized positionally in several tests. Removing sprint fields changes the member order. Update every aggregate initializer in `test_spin_controller.cpp`, `test_wash_cycle_coordinator.cpp`, and `test_panel_controller.cpp`.

### 5.2 `VibrationConfig`

Add hysteresis and warning debounce fields at the end of the struct so existing positional initializers remain source-compatible until they are explicitly updated.

```cpp
struct VibrationConfig
{
    uint16_t sample_period_ms{20};
    uint8_t window_samples{10};
    uint16_t motion_threshold{400};
    uint16_t accel_warning_threshold{5500};
    uint16_t trip_threshold{11000};
    uint16_t shock_threshold{14000};
    uint16_t sustained_trip_duration_ms{1000};
    uint16_t accel_warning_clear{4000};
    uint16_t warning_debounce_ms{0};
};
```

    Threshold rationale:

    - `accel_warning_threshold` is 5500 LSB (ACCEL-only damping context) to react before the end-of-ramp resonance spike while tolerating normal duty vibration; trip and shock remain active in all phases.
    - `accel_warning_clear` is 4000 LSB (proportional hysteresis). Warning/damping is active only during ACCEL (and ACCEL-origin damping); trip and shock remain active during duty.
    - `warning_debounce_ms` is 0: the monitor already integrates a 200 ms peak-to-peak window, so zero debounce latches on the first window above threshold and cuts one window of reaction latency. Any nonzero value up to 200 ms is equivalent to requiring the next window. Critical shocks remain immediate through `shock_threshold`.
    - These thresholds must be validated with real wash loads before being treated as final.

### 5.3 Configuration invariants

Enforce at runtime in the `VibrationMonitor` constructor or `init()`:

```text
accel_warning_clear < accel_warning_threshold < trip_threshold
motion_threshold < accel_warning_threshold
warning_debounce_ms < sustained_trip_duration_ms
damping_min_off_ms < damping_max_off_ms
accel_motor_on_ms < accel_max_wall_ms
```

Do not use `static_assert` for these values. `VibrationConfig` and `SpinConfig` are runtime configuration objects, not compile-time constants. Use a validation function returning `bool` or an `enum class` result. If validation fails, clamp to safe defaults and expose the invalid state for tests.

---

## 6. Detailed Design

### 6.1 `VibrationMonitor` warning hysteresis

Replace the single-window warning assignment in `process_sample()` with latching set/clear logic.

```cpp
// Pseudocode inside process_sample(), after current_vib_ is computed.
if (current_vib_ >= config_.accel_warning_threshold)
{
    if (warning_since_ms_ == 0) {
        warning_since_ms_ = now;
    }
    if ((now - warning_since_ms_) >= config_.warning_debounce_ms) {
        is_warning_ = true;
    }
}
else if (current_vib_ < config_.accel_warning_clear)
{
    is_warning_ = false;
    warning_since_ms_ = 0;
}
else
{
    // Hysteresis hold: keep the previous warning state.
}
```

    Add a private member :

```cpp uint32_t warning_since_ms_{0};
```

`reset()` must clear `warning_since_ms_` together with the existing flags.

    Behavior requirements :

    -Warning remains latched between `accel_warning_clear` and `accel_warning_threshold` during ACCEL
    - damping mode.-
    A new warning
        requires `warning_debounce_ms` of continuous above - threshold samples.-
Critical shock still trips immediately
on
the first window above `shock_threshold`.- Sustained critical trip behavior is unchanged.- `is_warning()` remains
a `const` method and the interface does
not change
.

## #6.2 `VibrationDampingPolicy`

Create
a pure class with
no
HAL dependencies.

```cpp enum class DampingAction : uint8_t {
    NONE = 0,
    PAUSE,
    RESUME,
    ESCALATE
};

struct DampingInput
{
    bool sensor_ok;
    bool warning;
    bool critical;
    uint32_t now_ms;
};

struct DampingConfig
{
    uint32_t min_off_ms{800};
    uint32_t max_off_ms{8000};
    uint32_t sensor_timeout_ms{1000};
};

class VibrationDampingPolicy
{
public:
    explicit VibrationDampingPolicy(const DampingConfig& config = DampingConfig{});

    DampingAction evaluate(const DampingInput& input, bool motor_energized, bool damping_active) const;

    bool can_resume(const DampingInput& input, uint32_t damping_elapsed_ms) const;

private:
    DampingConfig config_;
};
```

        Decision order in `evaluate()`:

    |
    Priority
    |
    Condition
    |
    Action
    |
    |
    -- - : |
           -- -|
           -- -|
           |
           1 | `critical == true` | `ESCALATE` |
           |
           2 | `sensor_ok == false` and sensor timeout exceeded
           | `ESCALATE` |
           |
           3 | `warning == true` and `motor_energized == true` | `PAUSE` |
           |
           4 | `damping_active == true` and `can_resume()` | `RESUME` |
           |
           5 | `damping_active == true` and `max_off_ms` exceeded
           | `ESCALATE` |
           |
           6 |
           Otherwise
           | `NONE` |

`can_resume()`:

```cpp return input.sensor_ok
    && !input.warning && !input.critical && damping_elapsed_ms >= config_.min_off_ms;
```

    The policy does not know about motor or
    pump pins. `SpinController` maps `PAUSE` to `motor_.stop()` and `RESUME` to `motor_.rotate_clockwise()`
                                                                        .

                                                                    ## #6.3 `AccelTimeline`

                                                                    Create a pure timer helper.

```cpp class AccelTimeline
{
public:
    void start(uint32_t now_ms, uint32_t motor_on_target_ms, uint32_t wall_cap_ms);
    void on_motor_on(uint32_t now_ms);
    void on_motor_off(uint32_t now_ms);
    bool is_complete(uint32_t now_ms) const;
    uint32_t motor_on_elapsed_ms() const;

private:
    uint32_t start_ms_{0};
    uint32_t last_tick_ms_{0};
    uint32_t motor_on_elapsed_ms_{0};
    uint32_t motor_on_target_ms_{20000};
    uint32_t wall_cap_ms_{40000};
    bool motor_on_{false};
};
```

        Rules :

    - `motor_on_elapsed_ms_` accumulates only while the motor is commanded
        on.- `VIBE_DAMPING` time does not count toward `motor_on_elapsed_ms_`.- `is_complete()` returns true when either
            the motor -
        on target is reached or
    the wall - clock cap is exceeded.- The wall -
        clock cap prevents a heavy load with repeated damping pauses from remaining in ACCEL forever.

        ## #6.4 `SpinController` state transitions

            New enum :

```cpp enum class SpinSubPhase : uint8_t {
                IDLE = 0,
                CLUTCH_ENGAGE,
                ACCEL,
                VIBE_DAMPING,
                DUTY_RUN_ON,
                DUTY_RUN_OFF,
                COAST_DOWN,
                PAUSE_COASTING,
                PAUSED,
                STOP_COASTING,
                RETRY_COASTING
            };
```

    New private members :

```cpp SpinSubPhase damping_return_phase_{SpinSubPhase::ACCEL};
uint32_t damping_phase_elapsed_ms_{0};
uint32_t damping_start_ms_{0};
AccelTimeline accel_timeline_;
```

Do not reuse `resume_target_phase_` for damping. It already serves clutch re-engagement and pause/resume semantics. Overloading it creates conflicts when the user pauses during `VIBE_DAMPING`.

`start()` flow:

```text
level_ = level
duty_run_duration_ms_ = duration_sec * 1000
is_active_ = true
is_paused_ = false
is_finished_ = false
has_error_ = false
unbalance_retries_ = 0
drain_pump_.turn_on()
motor_.stop()
sub_phase_ = CLUTCH_ENGAGE
resume_target_phase_ = ACCEL
phase_start_ms_ = now
```

`CLUTCH_ENGAGE` completion:

```text
sub_phase_ = ACCEL
phase_start_ms_ = now
accel_timeline_.start(now, config_.accel_motor_on_ms, config_.accel_max_wall_ms)
motor_.rotate_clockwise()
```

`ACCEL` case:

```text accel_timeline_.on_motor_on(now)

    if (accel_timeline_.is_complete(now))
{
    accel_timeline_.on_motor_off(now);
    transition_to_duty_run();
}
```

`VIBE_DAMPING` entry, only from motor - energized phases :

```text damping_return_phase_ = sub_phase_ damping_phase_elapsed_ms_ =
                           now - phase_start_ms_ damping_start_ms_ = now motor_.stop() sub_phase_ = VIBE_DAMPING
```

`VIBE_DAMPING` update :

```text vibration_monitor_->update()

                               if (critical){enter_retry_or_stop(now) return }

    DampingInput input
{
    sensor_ok, is_warning(), is_critical_unbalance(), now
}

if (policy.evaluate(input, false, true) == DampingAction::ESCALATE) {
    enter_retry_or_stop(now) return
}

if (policy.can_resume(input, now - damping_start_ms_)) {
    sub_phase_ = damping_return_phase_ phase_start_ms_ = now - damping_phase_elapsed_ms_ motor_.rotate_clockwise()
}
```

If `damping_return_phase_` is `DUTY_RUN_ON`:

- On entry, add `(now - duty_run_start_ms_)` to `duty_run_elapsed_before_pause_ms_`.
- On resume, set `duty_run_start_ms_ = now`.
- Set `phase_start_ms_ = now - damping_phase_elapsed_ms_` so the remaining duty ON time continues.

If `damping_return_phase_` is `ACCEL`, `AccelTimeline` already tracks only motor-on time, so damping time is naturally excluded.

`DUTY_RUN_ON` / `DUTY_RUN_OFF`:

- Keep the existing 4 s ON / 4 s OFF pattern.
- `DUTY_RUN_ON` can enter `VIBE_DAMPING`.
- `DUTY_RUN_OFF` must not enter `VIBE_DAMPING`; the motor is already off.
- Critical unbalance can still be evaluated in both states.

`RETRY_COASTING` completion:

```text
vibration_monitor_->reset()
sub_phase_ = CLUTCH_ENGAGE
resume_target_phase_ = ACCEL
phase_start_ms_ = now
```

`resume()` after pause:

- Keep existing behavior of restarting the spin ramp from a safe state.
- Set `resume_target_phase_ = ACCEL`.
- Re-engage clutch before applying motor power.

`pause()` during `VIBE_DAMPING`:

- Stop the motor (already stopped).
- Enter `PAUSE_COASTING`.
- Keep the drain pump on until coast-down completes.
- On `resume()`, restart from `CLUTCH_ENGAGE` → `ACCEL`.

`stop()` during `VIBE_DAMPING`:

- Enter `STOP_COASTING`.
- Keep the pump on for the full 18 s coast-down window.
- Finish or latch error according to the existing retry logic.

### 6.5 Vibration monitor update policy

The current code updates the monitor only when `is_spinning` is true. That is not sufficient for `VIBE_DAMPING`.

Required behavior:

| State | Sample monitor | Evaluate critical | Evaluate warning for damping |
|---|---:|---:|---:|
| `ACCEL` | Yes | Yes | Yes |
| `VIBE_DAMPING` | Yes | Yes | Yes, to resume |
| `DUTY_RUN_ON` | Yes | Yes | Yes |
| `DUTY_RUN_OFF` | Yes | Yes | No |
| `COAST_DOWN` | No | No | No |
| `PAUSE_COASTING` | No | No | No |
| `PAUSED` | No | No | No |
| `STOP_COASTING` | No | No | No |
| `RETRY_COASTING` | No | No | No |

Implementation recommendation:

```cpp
bool monitor_active =
    sub_phase_ == SpinSubPhase::ACCEL ||
    sub_phase_ == SpinSubPhase::VIBE_DAMPING ||
    sub_phase_ == SpinSubPhase::DUTY_RUN_ON ||
    sub_phase_ == SpinSubPhase::DUTY_RUN_OFF;

if (vibration_monitor_ != nullptr && monitor_active) {
    vibration_monitor_->update();

    if (vibration_monitor_->is_critical_unbalance()) {
        enter_retry_or_stop(now);
        return;
    }

    bool motor_energized = sub_phase_ == SpinSubPhase::ACCEL || sub_phase_ == SpinSubPhase::DUTY_RUN_ON;

    if (motor_energized && vibration_monitor_->is_warning()) {
        enter_vibe_damping(now);
        return;
    }
}
```

This fixes the deadlock: `is_warning_` is latching in ACCEL-damping mode, so `VIBE_DAMPING` must continue calling `update()` until a new window below `accel_warning_clear` clears it.

### 6.6 Sensor failure fail-safe

The existing firmware keeps spinning if the accelerometer stops responding. This refactor should not preserve that blind spot.

Recommended behavior:

- Track `sensor_ok_since_ms_` in `SpinController`.
- If `is_sensor_ok()` is false for more than `damping_sensor_timeout_ms` while in `ACCEL`, `DUTY_RUN_ON`, or `VIBE_DAMPING`, enter `RETRY_COASTING` or `STOP_COASTING` according to the retry counter.
- If the sensor recovers before the timeout, clear the failure timer and continue.
- Do not wait forever in `VIBE_DAMPING` when the sensor has failed.

This is a safety improvement and a liveness guarantee. If a shorter or longer timeout is preferred, make it configurable;
do {
not hard - code it in the state machine.

               -- -

               ##7. Retry and Recovery Tuning

               ## # `SpinController::max_unbalance_retries` — dry redistribution retries

                   ** Location
    : ** `src / controllers /
               spin_controller
                   .hpp` L61** Current default : ** `1` ** Proposed default
    : ** `2`–`3`

      This controls how many times `SpinController` executes a full `RETRY_COASTING` cycle before
          latching `has_error_`.Each retry stops the motor,
    coasts to standstill, resets the vibration monitor,
    and restarts from the acceleration phase.

        With `VIBE_DAMPING` acting as the first -
        line response to imbalance,
    the cases that escalate to `RETRY_COASTING` should be genuinely severe.Giving the system two or
        three dry retries before a hydraulic recovery is a reasonable increase in
            resilience.Each retry costs one `coast_down_ms` window plus the acceleration
                ramp.With `coast_down_ms` increased to 18 s,
    the worst - case time increases,
    so start with* * 2 * *and evaluate* * 3 * *only after real -
        load testing.

        ## # `WashCycleCoordinator::unbalance_agitate_sec` — hydraulic recovery agitation time

            ** Location:** `src/fsm/wash_cycle_coordinator.hpp` L72  
**Current default:** `30 s`  
**Proposed value:** `120 s`

This is the agitation duration used during a hydraulic recovery. When `SpinController` exhausts all dry retries, the coordinator refills the tub to `unbalance_fill_level`, runs the agitator, drains, and retries the spin.

Thirty seconds is usually too short for a heavy concentrated load, such as a tightly folded blanket or wet jeans, to redistribute meaningfully. One hundred and twenty seconds gives the agitator four times as long to untangle and spread the load before the next spin attempt. This improves the probability of recovery without user intervention, at the cost of a longer worst-case cycle.

### `SpinConfig::coast_down_ms` — full-stop coast-down window

**Location:** `src/controllers/spin_controller.hpp` L58  
**Current default:** `10000`  
**Proposed default:** `18000`

The current 10 s coast-down window is insufficient to guarantee a complete stop when the machine is at full rotation. At approximately 750–768 RPM, the drum and load retain significant angular momentum. If the pump/brake is released or the motor is re-engaged while the drum is still rotating, the transmission and suspension can receive an unnecessary mechanical shock.

Increase `coast_down_ms` to **18000 ms**. This applies to:

- Normal completion before pump release
- `PAUSE_COASTING`
- `STOP_COASTING`
- `RETRY_COASTING`

The longer window adds up to 8 s per coast event. Combined with additional dry retries, this increases worst-case cycle time; that is acceptable because the alternative is releasing the transmission before the drum has actually stopped. Tests may continue to use shorter coast-down values for speed, but production defaults must use 18 s.

### Escalation hierarchy

```text
Vib < accel_warning_clear (ACCEL mode)
    Normal run. Motor ON. No action.

accel_warning_clear <= Vib < accel_warning_threshold (ACCEL mode)
    Hysteresis hold. No change to motor.

Vib >= accel_warning_threshold and motor energized in ACCEL
    VIBE_DAMPING. Motor OFF. Resume when Vib < accel_warning_clear (ACCEL mode)
    and damping_min_off_ms has elapsed.

Vib >= trip_threshold sustained, or >= shock_threshold
    RETRY_COASTING. Full stop, coast 18 s, reset monitor, restart ACCEL.
    Consumes one max_unbalance_retries slot.

Retries exhausted
    Hydraulic recovery:
    refill, agitate 120 s, drain,
        retry spin.Consumes one max_unbalance_recoveries
                slot.

            Recoveries exhausted Error latched.Cycle aborted.
```

            -- -

            ##8. Files to Change

            | File | Change | | -- -| -- -| | `src / controllers / vibration_monitor.hpp` |
            Add `accel_warning_clear / threshold`,
        ACCEL - damping mode, `warning_debounce_ms`, `warning_since_ms_`,
        constructor validation | | `src / controllers / vibration_monitor.cpp` |
            Implement hysteretic warning set / clear and debounce |
            | `src / controllers / interfaces / i_vibration_monitor.hpp` | No interface change |
            | `src / controllers / vibration_damping_policy.hpp` | New pure policy class |
            | `src / controllers / vibration_damping_policy.cpp` | New decision logic |
            | `src / controllers / accel_timeline.hpp` | New pure ACCEL timer |
            | `src / controllers / accel_timeline.cpp` | New motor - on accumulator and wall cap |
            | `src / controllers / spin_controller.hpp` | Remove sprint types and fields;
    add `ACCEL`, `VIBE_DAMPING`, damping fields, `AccelTimeline`;
    update `SpinConfig` | | `src / controllers / spin_controller.cpp` | Remove sprint state machine;
    implement ACCEL, damping, monitor update, timing accounting,
        fail - safes | | `src / fsm / wash_cycle_coordinator.hpp` | Change `unbalance_agitate_sec` default to 120 s |
            | `test / test_vibration_monitor.cpp` | Add hysteresis,
        debounce, invalid - config,
        sensor - health tests | | `test / test_vibration_damping_policy.cpp` | New decision - table tests |
            | `test / test_accel_timeline.cpp` | New timer tests | | `test / test_spin_controller.cpp` |
            Replace sprint tests with ACCEL / damping tests;
} update `SpinConfig` initializers |
| `test/test_wash_cycle_coordinator.cpp` | Update `SpinConfig` aggregate initializer |
| `test/test_panel_controller.cpp` | Update `SpinConfig` aggregate initializer |
| `docs/vibration_analysis.md` | Replace the 150–350 RPM band with the measured 200–380 RPM band and reference the RPM report |
| `docs/balance_ring_analysis.md` | Note that the sprint profile has been replaced by ACCEL + adaptive damping |
| `API.md` | Regenerate after public API changes |

---

## 9. Test Plan

### 9.1 Host tests

Run:

```bash
make test
```

Required new coverage:

**`test_vibration_monitor.cpp`**

- Warning sets above `warning_threshold`.
- Warning remains latched between `accel_warning_clear` and `accel_warning_threshold` during ACCEL-damping mode.
- Warning clears below `warning_clear`.
- `warning_debounce_ms = 0` latches on the first 200 ms window; a nonzero debounce requires continuous windows above threshold.
- `reset()` clears warning and debounce state.
- Invalid `warning_clear >= warning_threshold` is rejected or clamped.
- Sensor failure sets `is_sensor_ok()` false without silently clearing warning.
- Critical shock still trips immediately.

**`test_vibration_damping_policy.cpp`**

- Critical input returns `ESCALATE`.
- Warning with motor energized returns `PAUSE`.
- Warning with motor de-energized does not return `PAUSE`.
- Damping active, warning clear, before `min_off_ms` returns `NONE`.
- Damping active, warning clear, after `min_off_ms` returns `RESUME`.
- Damping active, `max_off_ms` exceeded returns `ESCALATE`.
- Sensor timeout returns `ESCALATE`.

**`test_accel_timeline.cpp`**

- Motor-on time accumulates only while motor is on.
- Motor-off time does not accumulate.
- Completion occurs at `accel_motor_on_ms`.
- Wall-clock cap completes the phase even if motor-on time is short.
- Restart resets all state.

**`test_spin_controller.cpp`**

- `start()` transitions from `CLUTCH_ENGAGE` to `ACCEL`.
- `ACCEL` transitions to `DUTY_RUN_ON` after the motor-on target.
- Warning during `ACCEL` enters `VIBE_DAMPING`.
- Warning during `DUTY_RUN_ON` enters `VIBE_DAMPING`.
- Warning during `DUTY_RUN_OFF` does not enter `VIBE_DAMPING`.
- `VIBE_DAMPING` calls `vibration_monitor_->update()` every update cycle. This test is mandatory to prevent the latching deadlock.
- Warning clear after `damping_min_off_ms` returns to `ACCEL` or `DUTY_RUN_ON` correctly.
- Critical during `VIBE_DAMPING` enters `RETRY_COASTING` or `STOP_COASTING`.
- `damping_max_off_ms` exceeded escalates.
- Sensor failure during damping escalates.
- `pause()` during `VIBE_DAMPING` and `resume()` preserve safe behavior.
- `RETRY_COASTING` restarts in `ACCEL`.
- Balanced load never enters `VIBE_DAMPING`.
- Duty-run elapsed time excludes damping time.

### 9.2 Firmware build

Run:

```bash
make build
```

Check flash and RAM usage on the ATmega328P. Removing the sprint tables should offset part of the cost of the new states and policy classes.

### 9.3 Hardware validation

Run instrumented tests with 50 Hz telemetry for:

1. Empty drum.
2. Light unbalanced blanket.
3. Heavy concentrated blanket.
4. Wet jeans, to exercise conical precession.
5. Normal distributed load.

Acceptance observations:

- Balanced loads do not enter `VIBE_DAMPING` unnecessarily.
- Heavy loads do not reach the critical trip during ACCEL.
- `VIBE_DAMPING` never exceeds `damping_max_off_ms`.
- No sensor-failure deadlock.
- No motor re-engagement before `damping_min_off_ms`.
- The drum does not dwell in the 200–380 RPM band for more than a few seconds.
- The drain pump remains ON during spin and coast-down.
- Cycle time does not grow more than roughly 15–20% for normal loads.

---

## 10. Ordered Implementation Steps

1. **Capture baseline behavior.**  
   Record current cycle times and vibration peaks from `logs/test9`, `test10`, `test11`, and `logs/balance-ring/test_07`. Use these as regression references.

2. **Add `warning_clear` and debounce to `VibrationMonitor`.**  
   Write failing tests first in `test_vibration_monitor.cpp`, then implement hysteretic set/clear and debounce.

3. **Add `VibrationDampingPolicy`.**  
   Write the decision-table tests first. Implement without HAL dependencies.

4. **Add `AccelTimeline`.**  
   Write tests for motor-on accumulation, wall cap, and reset. Implement as a pure class.

5. **Refactor `SpinConfig` and `SpinSubPhase`.**  
   Remove sprint fields and sprint enumerators. Add ACCEL, damping parameters, and the new `coast_down_ms` default.

6. **Refactor `SpinController::start()` and `update()`.**  
   Replace sprint branches with `CLUTCH_ENGAGE` → `ACCEL` → `DUTY_RUN_ON/OFF`. Add `VIBE_DAMPING` entry and exit logic.

7. **Fix monitor integration and timing accounting.**  
   Ensure `vibration_monitor_->update()` runs in `VIBE_DAMPING`. Exclude damping time from duty-run elapsed time. Preserve ACCEL motor-on time across damping.

8. **Update retry and pause/resume paths.**  
   `RETRY_COASTING` must return to `ACCEL`. `resume()` must set `resume_target_phase_ = ACCEL`. `pause()` during damping must keep the pump on through coast-down.

9. **Add fail-safes.**  
   Implement sensor timeout escalation and `damping_max_off_ms` escalation.

10. **Update coordinator defaults.**  
    Set `max_unbalance_retries` to 2, `unbalance_agitate_sec` to 120 s, and `coast_down_ms` to 18 s.

11. **Update all tests and fixtures.**  
    Remove sprint fixtures, update aggregate initializers, add ACCEL and damping tests. Run `make test`.

12. **Build for AVR.**  
    Run `make build` and inspect memory usage.

13. **Run hardware validation and calibrate.**  
    Tune `warning_threshold`, `warning_clear`, `accel_motor_on_ms`, `damping_min_off_ms`, and `damping_max_off_ms` from real telemetry. Only then decide whether to use 3 dry retries.

14. **Update documentation and regenerate `API.md`.**  
    Update `vibration_analysis.md`, `balance_ring_analysis.md`, and the RPM report cross-reference.

---

## 11. Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| `VIBE_DAMPING` deadlock because monitor is not updated | Motor remains off forever | Mandatory test that `update()` is called during damping |
| Warning latches during sensor failure | Cycle stalls | Sensor timeout escalation to retry/stop |
| Re-engagement during resonance | Mechanical jolt | `damping_min_off_ms`, hysteresis, and hardware validation |
| ACCEL ends too early under heavy load | Drum enters duty run below cruise speed | Motor-on accumulation plus `accel_max_wall_ms`; validate with heavy loads |
| ACCEL ends too late | Longer cycle, unnecessary motor-on time | Calibrate `accel_motor_on_ms` with telemetry |
| `coast_down_ms = 18 s` increases cycle time | Longer worst-case recovery | Accept for safety; measure with real cycles |
| `unbalance_agitate_sec = 120 s` increases recovery time | Longer worst-case cycle | Keep separate from damping validation; measure recovery success rate |
| Aggregate initialization breakage | Compile/test failures | Update every `SpinConfig` initializer in the same commit |
| Thresholds too aggressive | Unnecessary pauses on normal loads | Calibrate `warning_threshold` and `warning_clear` with logs |

---

## 12. Acceptance Criteria

- [ ] No `SPRINT_ON`, `SPRINT_OFF`, `SprintStep`, or sprint profile remains in production code.
- [ ] `ACCEL` is the only acceleration phase.
- [ ] `VIBE_DAMPING` is the only adaptive pause mechanism during acceleration and motor-on cruise.
- [ ] `is_warning()` uses hysteresis with `warning_clear < warning_threshold`.
- [ ] `warning_debounce_ms` semantics are documented (0 = first window; nonzero = continuous windows).
- [ ] `VIBE_DAMPING` continues sampling the vibration monitor.
- [ ] Critical unbalance is evaluated during `VIBE_DAMPING`.
- [ ] Sensor failure cannot leave the motor off indefinitely.
- [ ] `damping_max_off_ms` escalates safely.
- [ ] `ACCEL` completion is based on accumulated motor-on time, with a wall-clock cap.
- [ ] `duty_run_elapsed_before_pause_ms_` excludes damping time.
- [ ] `max_unbalance_retries` defaults to 2.
- [ ] `unbalance_agitate_sec` defaults to 120 s.
- [ ] `coast_down_ms` defaults to 18 s.
- [ ] `make test` passes.
- [ ] `make build` succeeds with acceptable AVR memory usage.
- [ ] Hardware tests show no dwelling in the 200–380 RPM band.
- [ ] Balanced loads do not trigger unnecessary damping.
- [ ] Heavy loads do not reach the critical trip during ACCEL.
- [ ] Documentation and `API.md` reflect the new architecture.

---

## 13. Documentation Updates

- Update `docs/vibration_analysis.md`:
  - Replace the 150–350 RPM resonance estimate with the measured 200–380 RPM band.
  - Reference `docs/report_rpm_resonance.md` for the measurement method and limitations.
  - Remove the sprint schedule as the active production strategy.
  - Document the ACCEL + vibration damping architecture.

- Update `docs/balance_ring_analysis.md`:
  - Note that the sprint profile was replaced by ACCEL + adaptive damping.
  - Keep the historical sprint measurements as background, not as current configuration.

- Regenerate `API.md`:
  - `SpinConfig` fields changed.
  - `SpinSubPhase` enumerators changed.
  - New classes `VibrationDampingPolicy` and `AccelTimeline` added.

- Update `README.md` if it describes the sprint profile as an active feature.

---

## 14. Rollback Plan

Before implementation, create a Git tag on the current working baseline.

If the refactor must be rolled back:

1. Revert the commit or merge that introduced `ACCEL` and removed sprints.
2. Restore the previous `SpinConfig` and `SpinSubPhase` definitions.
3. Restore the sprint tests.
4. Keep the `VibrationMonitor` hysteresis and `coast_down_ms` changes only if they are independently validated;
  otherwise revert them as part of the same change set.

      Do not keep both sprint and
      ACCEL paths behind a production runtime flag.Maintaining two acceleration strategies doubles the test matrix and
      reintroduces the open - loop behavior the refactor is intended to remove.
