# Adaptive Vibration Damping — Architecture Plan

> **Historical design document:** The implemented ACCEL-only tuning is documented in `docs/accel_vibe_damping_refactor_plan.md` (warning 3500/2000 LSB, damping min-off 2500 ms, sustained trip 800 ms). The 5500/4000 LSB values below are the original proposal and are kept for rationale.

## Background

The current spin cycle uses a **statically calibrated sprint profile** (four ON/OFF timed pulses)
to navigate the mechanical resonance band that the drum passes through during acceleration
(approximately 150–350 RPM). Those sprint timings were determined empirically through ten
instrumented test runs, logged and documented in `docs/vibration_analysis.md`.

The fundamental limitation of this approach: the sprint schedule is **open-loop**. It was tuned
against a specific load condition and can only approximate the correct behavior for a different
load distribution or weight. In practice, the same sprint profile that keeps a heavy concentrated
blanket stable may be unnecessarily conservative for a light, well-distributed load — and vice
versa: a load that happens to resonate slightly differently than the reference may still trigger
severe shaking even with the calibrated profile.

The vibration sensor (`VibrationMonitor` backed by an MPU-6050 on the top frame) already has the
data to close this loop. The `is_warning()` flag exists in the codebase but is **currently unused
by `SpinController`**.

---

## Problem Statement

The resonance event always occurs **during the sprint (acceleration) phase**, never during
`DUTY_RUN`. Once the drum crosses the resonance band and reaches cruising speed, vibration drops
to stable levels for the rest of the cycle.

The sprints attempt to cross this band by pulsing the motor: short ON bursts followed by timed
OFF pauses that allow the drum to slow slightly and the laundry to redistribute before the next
pulse. However:

- If the OFF pause is **too short**: the drum does not slow enough to allow redistribution.
- If the OFF pause is **too long**: the drum drops back into the 200–250 RPM natural frequency
  band while freewheeling. When the motor re-energizes with the drum wobbling at resonance, a
  severe dynamic jolt occurs (observed in test runs 7 and 8).
- Either scenario is **dependent on load**, which varies every wash.

**The core issue**: the sprint timing is calibrated against physics that changes every cycle.

---

## Proposed Solution: Sensor-Driven Vibration Damping (`VIBE_DAMPING`)

Replace the rigid sprint schedule with a **closed-loop motor governor** driven by the
`VibrationMonitor`. The motor is kept ON as long as vibration is within safe limits. When
vibration exceeds the warning threshold, the motor is cut. The motor re-engages as soon as
vibration falls back to a safe level.

This eliminates the need to guess the correct OFF duration: the **physics of the load** dictates
when the drum has decelerated enough for redistribution, and the sensor reports it.

### Why deceleration resonance is not a concern here

The deceleration resonance problem in the test runs arose because a **fixed OFF timer** was too
long, allowing the drum to coast all the way down into the 200–250 RPM band. With sensor-driven
re-engagement, the motor restarts as soon as `Vib` drops below the clear threshold — which
happens well before the drum loses enough speed to fall into that band. The load redistributes
slightly, vibration drops, and the motor re-engages while the drum is still rotating safely
above the resonance zone.

---

## State Machine Change

### New state added to `SpinSubPhase`

```
VIBE_DAMPING  // Motor off, waiting for vibration to clear before resuming
```

### Transition logic (inside `SpinController::update()`)

The vibration check block currently only acts on `is_critical_unbalance()`. It will be extended:

```
if vibration_monitor is active and motor is spinning:
    if is_critical_unbalance()  → RETRY_COASTING  (existing behavior, unchanged)
    else if is_warning()        → VIBE_DAMPING     (new: soft pause until clear)

if sub_phase == VIBE_DAMPING:
    if NOT is_warning()         → restore resume_target_phase_ (motor back on)
```

### `resume_target_phase_` reuse

`SpinController` already stores `resume_target_phase_` to remember which phase to re-enter after
clutch re-engagement or resume from pause. The same field will be used by `VIBE_DAMPING` to
return to the exact phase that was interrupted (`SPRINT_ON` or `DUTY_RUN_ON`), **without
resetting any timing or progress counters**.

The `duty_run_elapsed_before_pause_ms_` accumulator already tracks time spent in `DUTY_RUN`
across interruptions; this will naturally cover `VIBE_DAMPING` interruptions at no extra cost.

---

## Hysteresis: Two Thresholds Instead of One

Using a single threshold for both stop and resume would cause **chattering**: the motor would
oscillate rapidly on and off at the boundary (e.g., Vib = 5500 → motor off → Vib = 5400 → motor
on → Vib = 5500 → repeat within a single 200 ms window).

The solution is a **hysteresis band** with two distinct thresholds:

| Threshold | Purpose | Suggested value |
|---|---|---|
| `warning_threshold` | Motor stop trigger (existing field) | **5 500 LSB** |
| `warning_clear` | Motor re-engage trigger (new field) | **3 500 LSB** |

The gap between them (2 000 LSB) provides a stable dead-band. The motor only restarts when
vibration has genuinely settled, not on a momentary dip below the stop point.

### `is_warning()` hysteresis behavior

The `VibrationMonitor::process_sample()` currently sets:

```cpp
is_warning_ = (current_vib_ >= config_.warning_threshold);
```

This will change to a **latching** pattern:

```
if current_vib >= warning_threshold  →  is_warning_ = true   (set)
if current_vib <  warning_clear      →  is_warning_ = false  (clear)
// Between warning_clear and warning_threshold: state is unchanged (hysteresis hold)
```

This change is entirely internal to `VibrationMonitor` and transparent to `SpinController`,
which continues to call `is_warning()` with no changes to its interface.

---

## Sprint Phase Implications and Implementation Options

The `DUTY_RUN` 4s ON / 4s OFF pattern was designed for **cruising speed maintenance**, not for
initial acceleration. Using it from the start would have two problems:

- **4s ON**: potentially too short for the drum to cross the resonance band (150–350 RPM) before
  the motor cuts out again.
- **4s OFF**: needlessly decelerates the drum when vibration is already within safe limits,
  wasting time for no mechanical reason.

`DUTY_RUN` is therefore not a candidate to replace the sprint phase directly. Two options exist.

---

### Option A — Keep Sprints, Add VIBE_DAMPING (low risk)

Sprints remain unchanged as the primary acceleration schedule. `VIBE_DAMPING` is added as an
**adaptive safety layer** on top.

```
CLUTCH_ENGAGE → SPRINT_ON ⇄ SPRINT_OFF (×N) → DUTY_RUN_ON ⇄ DUTY_RUN_OFF
                    ↕ VIBE_DAMPING (fires when is_warning during any sprint ON)
```

**Behavior:**

- Balanced load → VIBE_DAMPING never fires → identical to current behavior.
- Unbalanced load → VIBE_DAMPING cuts the motor mid-sprint and holds until vibration clears,
  then the sprint resumes from where it was interrupted.
- The sprint `on_ms` / `off_ms` timings become **maximum run-time envelopes** rather than
  primary redistribution mechanisms.

**Advantages:**

- Zero regression risk. The change is purely additive.
- Validates the VIBE_DAMPING logic in production without changing the acceleration profile.
- Good baseline to verify threshold calibration with real wash loads.

**Disadvantages:**

- Two overlapping mechanisms (sprint timing + sensor damping) coexist. The sprint configuration
  retains influence but becomes progressively less meaningful as VIBE_DAMPING handles the
  redistribution work.
- Sprint parameters still need to be maintained even though they are no longer the primary
  control knob.

> [!TIP]
> Option A is the recommended first step. It delivers immediate sensor-driven protection with
> minimal change surface and allows safe calibration of `warning_threshold` and `warning_clear`
> against real wash data before committing to a larger refactor.

---

### Option B — Replace Sprints with Fixed-Time ACCEL Phase (clean architecture)

The entire sprint profile is removed and replaced by a single new `SpinSubPhase::ACCEL` state:
motor runs continuously, `VIBE_DAMPING` is the **only** pause mechanism. After a fixed
`accel_duration_ms`, the controller transitions to `DUTY_RUN`.

```
CLUTCH_ENGAGE → ACCEL (motor ON, VIBE_DAMPING governs all pauses) → DUTY_RUN_ON ⇄ DUTY_RUN_OFF
```

**Behavior:**

- Balanced load → motor runs continuously through ACCEL, no pauses, fastest possible ramp-up.
- Unbalanced load → VIBE_DAMPING fires as many times as needed; the drum self-navigates through
  the resonance band at whatever pace the load redistribution allows.
- The transition to `DUTY_RUN` happens after `accel_duration_ms` (a single fixed parameter,
  e.g. 25–30 s), regardless of how many VIBE_DAMPING interruptions occurred.

> [!NOTE]
> `accel_duration_ms` counts **wall-clock time**, not motor-on time. VIBE_DAMPING pauses are
> included in the elapsed window. This keeps the implementation simple and avoids a second
> accumulator.

**Advantages:**

- Eliminates sprint profile configuration entirely (`SprintStep`, `sprint_count`, `on_ms`,
  `off_ms` arrays all go away).
- Single parameter controls the acceleration phase length.
- Fully adaptive: fast loads accelerate without any forced pauses; heavy unbalanced loads get
  as many sensor-driven redistributions as needed.
- Cleaner state machine with fewer states.

**Disadvantages:**

- Larger change surface. Requires a test run to validate that `accel_duration_ms` is long
  enough for the drum to reach cruising speed across all realistic load conditions.
- The hydraulic drainage assumption from earlier documentation (`docs/vibration_analysis.md`)
  described sprint pauses as useful for pump evacuation. This has since been re-evaluated:
  before significant centrifugal force builds, the outer tub is already dry and there is no
  meaningful water for the pump to evacuate during sprint pauses. **This concern is dismissed**
  and does not block Option B.

**Files changed vs Option A:**

| File | Option A | Option B |
|---|---|---|
| `vibration_monitor.hpp` | Add `warning_clear` | Same |
| `vibration_monitor.cpp` | Hysteretic `is_warning_` | Same |
| `spin_controller.hpp` | Add `VIBE_DAMPING` to enum | Add `VIBE_DAMPING` + `ACCEL`; remove sprint fields; add `accel_duration_ms` to `SpinConfig` |
| `spin_controller.cpp` | Extend guard block + add `VIBE_DAMPING` case | Same + replace sprint switch cases with `ACCEL` case; remove sprint state variables |

---

### Recommended progression

1. **Implement Option A** to validate the sensor-driven approach with minimal risk.
2. After confirming stable behavior across varied loads, **migrate to Option B** in a follow-up
   iteration, removing the sprint profile and replacing it with the `ACCEL` phase.

---

## Files to Change

### 1. `src/controllers/vibration_monitor.hpp` — `VibrationConfig`

Add `warning_clear` field:

```cpp
uint16_t warning_clear{3500};  //!< Vibration must drop below this to clear is_warning (hysteresis).
```

> [!IMPORTANT]
> `warning_clear` must always be set below `warning_threshold`. A runtime assert or static_assert
> should enforce this invariant.

### 2. `src/controllers/vibration_monitor.cpp` — `process_sample()`

Replace the single-threshold assignment with hysteretic set/clear logic using
`warning_threshold` and `warning_clear`.

### 3. `src/controllers/interfaces/i_vibration_monitor.hpp`

No interface changes required. `is_warning()` signature is unchanged; the hysteresis is an
internal implementation detail.

### 4. `src/controllers/spin_controller.hpp` — `SpinSubPhase`

Add `VIBE_DAMPING` enumerator after `RETRY_COASTING`.

### 5. `src/controllers/spin_controller.cpp` — `update()`

Extend the vibration guard block to handle `is_warning()` → `VIBE_DAMPING`, and add a
`VIBE_DAMPING` case in the `switch` statement to poll `is_warning()` and restore
`resume_target_phase_` when clear.

---

## Empirical Threshold Rationale

From `docs/vibration_analysis.md`:

| Zone | Vib range | Meaning |
|---|---|---|
| Normal | < 5 000 | Balanced load, stable cruising |
| Tolerable | 5 000 – 8 500 | Uneven wet clothes, no walking risk |
| Warning | 8 500 – 11 000 | Severe unbalance, heavy rocking |
| Critical | > 11 000 sustained / > 14 000 shock | Foot slip imminent |

The proposed `warning_threshold` of **5 500 LSB** sits at the lower boundary of the tolerable
zone — far enough below the mechanical risk zone to give the drum time to redistribute before
vibration climbs further, while high enough to avoid false triggers during normal ramp-up
transients (all well-calibrated test runs peaked below 5 300 during sprint motor switching).

The `warning_clear` of **3 500 LSB** sits comfortably inside the normal operating zone,
ensuring the motor re-engages only after genuine redistribution, not on a momentary lull.

> [!NOTE]
> Both thresholds should be verified with a dedicated test run on the actual machine with the
> new firmware before treating them as final. Threshold calibration is straightforward because
> `VibrationMonitor` exposes `get_vibration()` for real-time logging.

---

## Retry and Recovery Tuning

Alongside the VIBE_DAMPING changes, two existing configuration defaults should be adjusted to
give the system more opportunity to self-correct before latching an error.

### `SpinController::max_unbalance_retries` — dry redistribution retries

**Location:** `src/controllers/spin_controller.hpp` L61  
**Current default:** `1`  
**Proposed default:** `2`–`3`

This controls how many times `SpinController` will execute a full `RETRY_COASTING` cycle (stop,
coast to standstill, reset monitor, restart from Sprint 1) before giving up and latching
`has_error_`. Each retry gives the drum a chance to redistribute the load through inertia alone,
without adding water.

With VIBE_DAMPING now acting as a first-line response to imbalance, the cases that escalate to
`RETRY_COASTING` will be genuinely severe. Giving the system 2–3 dry retries (instead of 1)
before triggering a full hydraulic recovery is a reasonable increase in resilience without
meaningfully extending worst-case cycle time (each retry costs one `coast_down_ms` ≈ 10 s).

### `WashCycleCoordinator::unbalance_agitate_sec` — hydraulic recovery agitation time

**Location:** `src/fsm/wash_cycle_coordinator.hpp` L72  
**Current default:** `30 s`  
**Proposed value:** `120 s`

This is the agitation duration used during a **hydraulic recovery**: when `SpinController`
exhausts all dry retries, the `WashCycleCoordinator` refills the tub to `unbalance_fill_level`,
runs the agitator for `unbalance_agitate_sec`, drains, and retries the spin. The purpose is to
mechanically redistribute the load with water buoyancy assisting the tumbling action.

30 seconds is too short for a heavy concentrated load (e.g. a tightly folded blanket) to
redistribute meaningfully. 120 seconds gives the agitator four times as long to untangle and
spread the load before the next spin attempt, significantly improving the probability of a
successful recovery without user intervention.

> [!NOTE]
> `max_unbalance_recoveries` (currently `1`, in `WashCycleCoordinator`) is a separate counter
> from `max_unbalance_retries`. The hierarchy is:
>
> 1. VIBE_DAMPING fires → motor pauses momentarily → resumes (new, no retry consumed)
> 2. `is_critical_unbalance()` → `RETRY_COASTING` → dry restart (consumes one `unbalance_retries`)
> 3. `unbalance_retries` exhausted → hydraulic recovery (agitate + refill + drain + re-spin, consumes one `unbalance_recoveries`)
> 4. `unbalance_recoveries` exhausted → error latched, cycle aborted

---

## What Does Not Change

- `is_critical_unbalance()` logic and `RETRY_COASTING` behavior: untouched.
- `pause()` / `resume()` external API: untouched.
- `duty_run_elapsed_before_pause_ms_` accounting: naturally absorbs `VIBE_DAMPING` pauses.
- `IVibrationMonitor` interface: no changes.
- Sprint profile configuration: kept, sprint steps are not removed in this iteration.
- Motor protection: `VIBE_DAMPING` cuts the motor via `motor_.stop()`, same as any other pause.
  The drain pump remains ON throughout (consistent with existing behavior during active spin).

---

## Escalation Hierarchy (final)

```
Vib < warning_clear        Normal run   Motor ON, no action
warning_clear ≤ Vib < warning_threshold  Hysteresis hold  No change to motor
Vib ≥ warning_threshold    VIBE_DAMPING  Motor off; resume when Vib < warning_clear
Vib ≥ trip_threshold (sustained) or ≥ shock_threshold  RETRY_COASTING  Full stop + retry
Retries exhausted          Error latch   Cycle aborted
```
