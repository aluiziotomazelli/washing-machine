# Vibration telemetry and out-of-balance analysis

## Mechanical context

Top-load washing machines isolate drum vibrations using four hanging rod suspensions with springs and dampening pads. An internal liquid balance ring mounted along the upper rim of the spinning basket compensates for small clothes distributions once the drum reaches operating speed.

During acceleration, an eccentric mass in the basket produces a rotating centrifugal force:

$$F = m \cdot \omega^2 \cdot r$$

Where:
- $m$ is the unbalanced mass.
- $\omega$ is the rotational angular velocity.
- $r$ is the radial distance from the basket axis of rotation.

When the drum spins past the suspension critical resonance (typically between 150 and 350 RPM), unbalanced loads produce severe lateral and vertical displacement. If the horizontal forces exceed the static friction between the rubber feet and the floor, the cabinet shifts position. This phenomenon is known as machine walking:

$$F_{\text{lateral}} > \mu_s \cdot N$$

Where $\mu_s$ is the coefficient of static friction of the rubber feet against the floor, and $N$ is the normal force exerted by the machine mass on the ground.

## Instrumentation and sensor placement

An MPU-6050 3-axis MEMS accelerometer was integrated to measure cabinet vibration dynamics.

### Mounting position

The sensor is bolted to the rigid plastic top frame of the washing machine, directly above the metal chassis. This mounting location provides three concrete engineering advantages:

1. Lever-arm effect. The top frame sits at the maximum vertical distance from the floor contact point. Angular cabinet rocking on the rubber feet translates to the largest linear displacement and acceleration at the top ($a = h \cdot \ddot{\theta}$), maximizing sensor signal-to-noise ratio.
2. Short wiring run. The microcontroller, user interface, and low-voltage electronics are housed in the top console. Mounting the sensor nearby keeps I2C lines under 15 cm, preventing capacitive degradation and inductive noise pickup from motor wiring.
3. Serviceability. The entire top plastic console unclips as a single unit during tub and suspension maintenance, without requiring an umbilical harness running into the lower chassis.

### Hardware filtering and acquisition settings

- Scale: $\pm 4g$ (8192 LSB/g sensitivity).
- Digital Low Pass Filter (DLPF): configured on-chip to 44 Hz bandwidth (CONFIG register `0x03`). This rejects 60 Hz electrical hum and high-frequency motor inverter switching noise directly in silicon before ADC sampling.
- Bus interface: hardware I2C at 100 kHz with a 25 ms hardware timeout reset enabled in the AVR TWI peripheral.
- Acquisition rate: 50.0 Hz (20 ms period), logged through UART at 115200 baud.

## Measurement metric: 3D peak-to-peak envelope

To detect cabinet walking reliably without calibration or trigonometric overhead, the firmware tracks the peak-to-peak variation across all three axes over a rolling 200 ms window (10 samples at 50 Hz):

$$\Delta X = X_{\max} - X_{\min}$$

$$\Delta Y = Y_{\max} - Y_{\min}$$

$$\Delta Z = Z_{\max} - Z_{\min}$$

$$\text{Vib} = \Delta X + \Delta Y + \Delta Z$$

This metric offers specific mathematical properties for this application:

- DC gravity rejection. Static 1g Earth gravity appears as a constant offset on the vertical axis (around -7700 LSB in our mounting orientation). Taking the delta over 200 ms eliminates this constant without high-pass floating-point filters.
- Spatial orientation independence. Tub precession causes the unbalance force vector to sweep across the horizontal and vertical planes. Summing all three axis deltas captures total mechanical vibration energy regardless of minor mounting tilt.
- Linear integer arithmetic. Computing the metric requires only 16-bit subtractions and comparisons, taking less than 5 microseconds of AVR CPU time per cycle.

## Empirical test runs

Ten test runs were conducted on the actual machine to evaluate suspension dynamics across different acceleration profiles and load conditions. All telemetry was captured at 50 Hz and logged to the `logs/` directory.

### Baseline evaluation with legacy decreasing ramp

The original firmware used decreasing sprint pulses (6s down to 2s, with fixed pauses).

| Run | Log file | Level | Load condition | Avg Vib | Peak Vib | Cruise Avg | Behavior observed |
| :--- | :--- | :--- | :--- | :---: | :---: | :---: | :--- |
| 1 | `test1_empty_tub_low_level.csv` | Low (3 sprints) | Empty basket | 1967 | 5526 | 3020 | Quiet, steady, no cabinet movement |
| 2 | `test2_empty_tub_medium_level.csv` | Medium (5 sprints) | Empty basket | 2132 | 4710 | 3318 | Quiet, steady, no cabinet movement |
| 3 | `test3_unbalanced_light_blanket_medium_level.csv` | Medium (5 sprints) | Light dry blanket (one side) | 4688 | 12840 | 5160 | Heavy cabinet shake, nearly walked at 50.3s |
| 4 | `test4_unbalanced_light_blanket_low_level.csv` | Low (3 sprints) | Light dry blanket (one side) | 3269 | 10006 | 5568 | Heavy cabinet shake, nearly walked at 51.1s |
| 5 | `test5_unbalanced_heavy_blanket_medium_level.csv` | Medium (5 sprints) | Heavy folded blanket (one side) | 5367 | 15417 | 8450 | Cabinet walked on floor at 64.2s |

### Progressive acceleration ramp optimization

To eliminate resonance spikes and water drag, the sprint profile was updated to progressive increasing pulses with tailored motor pause intervals.

| Run | Log file | Sprint schedule (ON / OFF) | Load condition | Avg Vib | Peak Vib | Cruise Avg | Behavior observed |
| :--- | :--- | :--- | :--- | :---: | :---: | :---: | :--- |
| 6 | `test6_new_sprint_unbalanced_light_blanket.csv` | 5s/5s, 7s/5s, 9s/5s, 11s/5s | Light dry blanket (one side) | 1326 | 4468 | 1836 | Calm and smooth, but 11s ON felt excessively long |
| 7 | `test7_shorter_sprint_unbalanced_light_blanket.csv` | 3s/5s, 5s/5s, 7s/5s, 9s/5s | Light dry blanket (one side) | 1805 | 5044 | 3201 | 3s ON let tub stop completely, 9s ON ran into resonance |
| 8 | `test8_tailored_sprint_steps_unbalanced_light_blanket.csv` | 4s/3.5s, 5.5s/5s, 7s/4s, 8s/3s | Light dry blanket (one side) | 1003 | 5326 | 3331 | Initial inertia preserved, but S2 OFF lingered in resonance |
| 9 | `test9_finetuned_sprint_steps_unbalanced_light_blanket.csv` | 4s/3.5s, 5s/3.5s, 6s/4s, 7s/3s | Light dry blanket (one side) | 1616 | 5005 | 3520 | Fully stable, no deceleration resonance, seamless cruise |
| 10 | `test10_finetuned_sprint_empty_tub.csv` | 4s/3.5s, 5s/3.5s, 6s/4s, 7s/3s | Empty basket | 1366 | 3729 | 2255 | Peak dropped by 32.5% compared to run 1, very quiet |

## Experimental observations

### Acceleration ramp optimization and fluid dynamics

Pulsed spin cycles in top-load washing machines without tachometers serve two physical purposes:

1. Hydraulic drag prevention. Saturated clothes release several liters of water during initial drum rotation. A standard drain pump evacuates approximately 250 to 300 ml per second. Pauses between sprint pulses give the pump time to evacuate water from the outer tub before the next motor run, preventing the spinning basket from hitting pooled water and overloading the induction motor.
2. Radial clothes distribution. Progressive pulses allow wet items to distribute evenly against the perforated drum wall under moderate centrifugal force before full operating speed is engaged.

### Deceleration resonance

Test runs 7 and 8 revealed that washing machine suspensions can enter resonance during the coast-down (OFF) intervals:

- If an OFF pause is too long (such as 5.0 seconds in test 8 after sprint 2), the basket decelerates directly into the 200 to 250 RPM natural frequency band while freewheeling.
- When the motor re-energizes while the basket is wobbling at its natural frequency, electrical torque vector misalignment produces a severe dynamic jolt.
- Shortening the S2 pause from 5.0s to 3.5s in test 9 prevented the drum from dropping into the resonance band, allowing the motor to pick up the load while still smoothly rotating.

### Mechanics of cabinet walking

Test 5 captured the transition from severe shaking to physical foot displacement at 64.2 seconds:

```
62.2s: Vib = 11675
62.6s: Vib = 12337
63.2s: Vib = 12859
63.6s: Vib = 14257
64.0s: Vib = 14888
64.2s: Vib = 15417 (Cabinet foot slip occurred)
64.6s: Vib = 12329
65.0s: Vib = 11260
```

Axis breakdown during steady operation compared to the foot-slip window:

| Axis | Calm phase (40s to 45s) | Walking event (62s to 65s) | Dynamic increase |
| :--- | :---: | :---: | :---: |
| X (Vertical) | $\Delta = 782$ LSB | $\Delta = 3898$ LSB | 4.98x |
| Y (Lateral) | $\Delta = 1206$ LSB | $\Delta = 6706$ LSB | 5.56x |
| Z (Front-to-back) | $\Delta = 1784$ LSB | $\Delta = 5981$ LSB | 3.35x |

During the walking event, vertical acceleration on axis X swung between -5908 LSB ($0.72g$) and -9806 LSB ($1.20g$). When vertical acceleration reached $0.72g$, the effective normal force pressing the rubber feet against the floor dropped by 28%. At that exact moment, the lateral force on axis Y peaked at 6706 LSB. The reduced normal force dropped the static friction limit below the applied lateral force, causing the feet to slide across the floor.

### Duration vs single shocks

Isolated shock peaks occurred during sprint motor switching. For example, at 22.4s in test 5, a single 200 ms window peaked at 13747 without cabinet movement.

In contrast, the walking event at 62s to 65s sustained vibration above 11000 for over 3.0 continuous seconds before the feet broke friction. Cabinet walking requires continuous energy injection over several full rotation cycles to overcome suspension damping and chassis inertia.

## Final calibrated sprint schedule

The optimal sprint sequence established in test 9 and verified on empty basket in test 10 is configured in `SpinConfig`:

| Step | Motor run (ON) | Pump pause (OFF) | Engineering rationale |
| :---: | :---: | :---: | :--- |
| S1 | 4.0s | 3.5s | Overcomes belt and seal static friction; seats laundry without stalling |
| S2 | 5.0s | 3.5s | Extracts bulk water; re-engages before deceleration resonance can develop |
| S3 | 6.0s | 4.0s | Accelerates dry basket; 4s pause damps transient tub sway |
| S4 | 7.0s | 3.0s | Final velocity ramp; 3s pause delivers drum directly into cruising spin |

## Empirical threshold calibration

Based on all ten test runs, the vibration domain divides into four operational zones:

| Vibration index (Vib) | Operational status | Mechanical meaning |
| :--- | :--- | :--- |
| Below 5000 | Normal | Empty or properly balanced load. Steady cruising operation. |
| 5000 to 8500 | Tolerable imbalance | Uneven wet clothes. Noticeable cabinet oscillation, but well within static friction limits. No risk of walking. |
| 8500 to 11000 | Warning | Severe unbalance approaching mechanical suspension limits. Cabinet rocking heavily. |
| Above 11000 sustained (> 1.0s) or single shock above 14000 | Critical trip | Static friction failure threshold. Cabinet walking or suspension bottoming imminent. |
