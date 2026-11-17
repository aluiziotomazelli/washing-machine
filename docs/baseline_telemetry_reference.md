# Baseline Telemetry Reference (Step 1)

**Date Recorded:** 2026-11-16  
**Context:** Pre-refactor regression baseline (Sprint schedule in production).  
**Source Datasets:**
- `logs/test9_finetuned_sprint_steps_unbalanced_light_blanket.csv`
- `logs/test10_finetuned_sprint_empty_tub.csv`
- `logs/test11_finetuned_sprint_unbalanced_heavy_blanket.csv`
- `logs/balance-ring/test_07_almost-balanced-load_plus250ml.csv`

> **Historical baseline:** These captures predate the current tuning and were analyzed with the original 5500/4000 LSB proposal. Current production defaults are `accel_warning_threshold=3500`, `accel_warning_clear=2000`, `damping_min_off_ms=2500`, and `sustained_trip_duration_ms=800`; see `docs/accel_vibe_damping_refactor_plan.md`.

---

## 1. Summary Metrics

| Dataset | Load Profile | Total Duration | Samples | Max Vib (LSB) | Mean Vib (LSB) | P95 Vib (LSB) | Samples >= 5500 (Proposed Warn) | Samples >= 8500 (Legacy Warn) | Samples >= 11000 (Trip) |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `test10` | Empty Tub | 60.02 s | 3004 | 3,729 | 1,369.7 | 3,007 | 0 (0.00 s) | 0 (0.00 s) | 0 (0.00 s) |
| `test_07` | Almost Balanced (+250ml ring) | 52.97 s | 2651 | 3,884 | 1,131.1 | 2,997 | 0 (0.00 s) | 0 (0.00 s) | 0 (0.00 s) |
| `test9` | Unbalanced Light Blanket | 60.01 s | 3004 | 5,005 | 1,621.3 | 4,230 | 0 (0.00 s) | 0 (0.00 s) | 0 (0.00 s) |
| `test11` | Unbalanced Heavy Blanket | 65.01 s | 3254 | **11,865** | 3,114.1 | 10,596 | 610 (~12.20 s) | 450 (~9.00 s) | 90 (~1.80 s) |

---

## 2. Key Insights for ACCEL + Adaptive Damping Refactor

### 2.1 False Positive Immunity (Thresholds 5500 / 4000)
- **Empty tub (`test10`)** peaked at **3,729 LSB** (P95: 3,007 LSB).
- **Balanced load with ring (`test_07`)** peaked at **3,884 LSB** (P95: 2,997 LSB).
- **Light blanket (`test9`)** peaked at **5,005 LSB** (P95: 4,230 LSB).
- **Conclusion:** With the proposed `warning_threshold` = **5,500 LSB** and `warning_clear` = **4,000 LSB**, **neither the empty drum, balanced loads, nor mild unbalances will falsely trigger `VIBE_DAMPING`**. Normal operating vibrations stay cleanly below 4,000–5,500 LSB.

### 2.2 Blind Sprints Escalation in Heavy Unbalance (`test11`)
- Under the legacy sprint schedule, `test11` progressed through S1–S4 without sensory feedback:
  - 0–10 s (Clutch / S1): < 484 LSB
  - 10–25 s (S2 / S3 ramp): 2,600–3,973 LSB
  - 30–40 s (S4 ramp): 2,526–3,483 LSB
  - **40–65 s (Handover to continuous spin):** Motor was driven blindly into dynamic resonance, causing the envelope to surge to **11,865 LSB**, staying above 11,000 LSB for ~1.8 s and above 5,500 LSB for ~12.2 s.
- **Benefit of Refactor:** With sensor-driven `ACCEL`, the motor will be paused (`VIBE_DAMPING`) at ~40 s when the envelope crosses 5,500 LSB with 200 ms debounce, preventing the system from ever reaching the 11,000 LSB critical trip envelope.

---

## 3. Regression Acceptance Criteria (current tuning)

When validating the refactored implementation against hardware or replayed data:
1. **Empty tub:** Must complete ACCEL. The historical peak (3,729 LSB) can cross the current 3,500 LSB warning threshold; any damping pause must recover without escalation.
2. **Balanced load:** Must complete ACCEL. The historical peak (3,884 LSB) can cross the current 3,500 LSB warning threshold; any damping pause must recover without escalation.
3. **Light unbalance:** Must cross the 200–380 RPM band smoothly. Any damping pause, if triggered, must recover within `damping_min_off_ms` (2500 ms) and not escalate.
4. **Heavy unbalance:** Must engage `VIBE_DAMPING` at the current warning threshold (3,500 LSB) and mitigate amplitude, preventing unmitigated escalation to the 11,000 LSB trip threshold during acceleration.
