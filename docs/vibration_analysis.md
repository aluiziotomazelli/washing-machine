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

Five full spin test runs of 100 seconds each were executed on the actual washing machine to collect baseline and fault data. Telemetry was logged to CSV files in the `logs/` directory.

| Test run | Log file | Water level configuration | Basket load condition | Overall average Vib | Peak Vib | Cruising average (60s to 90s) | Cruising peak | Physical behavior observed |
| :--- | :--- | :--- | :--- | :---: | :---: | :---: | :---: | :--- |
| 1 | `test1_empty_tub_low_level.csv` | Low (3 sprints) | Empty basket | 1967 | 5526 | 3020 | 3913 | Quiet, steady, no cabinet movement |
| 2 | `test2_empty_tub_medium_level.csv` | Medium (5 sprints) | Empty basket | 2132 | 4710 | 3318 | 4555 | Quiet, steady, no cabinet movement |
| 3 | `test3_unbalanced_light_blanket_medium_level.csv` | Medium (5 sprints) | Light dry blanket (one side) | 4688 | 12840 | 5160 | 7558 | Strong cabinet shake, nearly walked at 50.3s |
| 4 | `test4_unbalanced_light_blanket_low_level.csv` | Low (3 sprints) | Light dry blanket (one side) | 3269 | 10006 | 5568 | 8294 | Strong cabinet shake, nearly walked at 51.1s |
| 5 | `test5_unbalanced_heavy_blanket_medium_level.csv` | Medium (5 sprints) | Heavy folded blanket (one side) | 5367 | 15417 | 8450 | 15417 | Cabinet walked slightly on floor at 64.2s |

## Experimental observations

### Transient resonance vs steady-state cruising

In all tests, the maximum peak vibration occurred during the motor acceleration ramp between 50s and 65s, rather than during steady-state top speed. Once the basket reached full speed (after 60s to 70s), gyroscopic stabilization reduced vibration by roughly 30% to 50% compared to the resonance peak.

The acceleration profile had a direct effect on resonance amplitude:
- In test 1 (low level, 3 sprints), the faster ramp caused an empty basket peak of 5526.
- In test 2 (medium level, 5 sprints), the progressive stages reduced the peak to 4710, a 15% reduction in mechanical shock.

### Mechanics of cabinet walking

Test 5 captured the transition from severe shaking to physical foot displacement at 64.2 seconds.

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

## Empirical threshold calibration

Based on the five test runs, the vibration domain divides into four distinct operational zones:

| Vibration index (Vib) | Operational status | Mechanical meaning |
| :--- | :--- | :--- |
| Below 5000 | Normal | Empty or properly balanced load. Steady cruising operation. |
| 5000 to 8500 | Tolerable imbalance | Uneven wet clothes. Noticeable cabinet oscillation, but well within static friction limits. No risk of walking. |
| 8500 to 11000 | Warning | Severe unbalance approaching mechanical suspension limits. Cabinet rocking heavily. |
| Above 11000 sustained (> 1.0s) or single shock above 14000 | Critical trip | Static friction failure threshold. Cabinet walking or suspension bottoming imminent. |
