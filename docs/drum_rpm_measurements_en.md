# Washing Machine Drum RPM Measurements

## Measurement assumptions

-   The source footage was recorded at **120 FPS** and played back in
    slow motion at **30 FPS**, corresponding to a **4:1 playback
    slowdown**.
-   The Kdenlive project timeline is configured at **30 FPS**.
    Therefore, the cut positions are resolved to 1/30-second increments
    on the project timeline.
-   Each interval between consecutive cuts represents one complete drum
    revolution, identified by the black reference mark passing the
    selected position.
-   RPM is calculated from the interval between cuts:

\[ RPM = `\frac{60 \times 4}{\Delta t_{\text{timeline}}}`{=tex} \]

where (`\Delta `{=tex}t\_{`\text{timeline}`{=tex}}) is the interval in
seconds on the slow-motion video timeline.

The cut positions may differ slightly from the exact instant at which
the mark crosses the reference position. The resulting RPM values should
therefore be treated as estimates, not precision tachometer readings.

## Acceleration ramp --- motor continuously on

The following values were calculated from the consecutive cut positions
in the Kdenlive project. Each row represents one drum revolution.

    Revolution   Start time   End time   Interval (s)     RPM
  ------------ ------------ ---------- -------------- -------
             1        6.100      8.933          2.833    84.7
             2        8.933     10.900          1.967   122.0
             3       10.900     12.567          1.667   144.0
             4       12.567     13.933          1.366   175.7
             5       13.933     15.133          1.200   200.0
             6       15.133     16.200          1.067   224.9
             7       16.200     17.200          1.000   240.0
             8       17.200     18.100          0.900   266.7
             9       18.100     18.967          0.867   276.8
            10       18.967     19.800          0.833   288.1
            11       19.800     20.533          0.733   327.4
            12       20.533     21.233          0.700   342.9
            13       21.233     21.967          0.734   327.0
            14       21.967     22.667          0.700   342.9
            15       22.667     23.300          0.633   379.1
            16       23.300     23.933          0.633   379.1
            17       23.933     24.500          0.567   423.3
            18       24.500     25.067          0.567   423.3
            19       25.067     25.633          0.566   424.0
            20       25.633     26.167          0.534   449.4
            21       26.167     26.667          0.500   480.0
            22       26.667     27.167          0.500   480.0
            23       27.167     27.667          0.500   480.0
            24       27.667     28.133          0.466   515.0
            25       28.133     28.600          0.467   513.9
            26       28.600     29.033          0.433   554.3
            27       29.033     29.467          0.434   553.0
            28       29.467     29.900          0.433   554.3
            29       29.900     30.333          0.433   554.3
            30       30.333     30.733          0.400   600.0
            31       30.733     31.133          0.400   600.0

### Acceleration-ramp observations

-   The onset of noticeable vibration was observed around **200--225
    RPM**.
-   The strongest resonance was observed around **300 RPM**.
-   Vibration began to decrease around **343--379 RPM**.
-   The drum reached approximately **423--424 RPM** for three
    consecutive revolutions, suggesting a temporarily stable region.
-   The ramp is not perfectly monotonic. Small variations may reflect
    actual speed changes, cut-position uncertainty, or both.

These observations describe the recorded run; they do not establish
exact resonance boundaries.

## Maximum speed --- motor continuously on

Measurement: **30 revolutions in 09:11** on the slow-motion video
timeline.

-   Timeline duration: 9 seconds + 11 frames at 30 FPS = 9.3667 seconds.
-   Actual elapsed time: 9.3667 / 4 = 2.3417 seconds.
-   Calculated speed: **768 RPM**.
-   Washing machine manual rating: **750 RPM**.
-   Difference from the manual rating: approximately **+2.4%**.

## Intermittent operation --- 4 seconds ON / 4 seconds OFF

These measurements were taken after the drum had already reached
cruising speed.

  -----------------------------------------------------------------------
  Measurement point     Time for 30 revolutions            Calculated RPM
                              on video timeline 
  ------------------- ------------------------- -------------------------
  End of the 4-second                  9.8667 s                     729.7
  ON period (`09:26`)                           

  End of the 4-second                 10.1667 s                     708.2
  OFF period                                    
  (`10:05`)                                     
  -----------------------------------------------------------------------

The arithmetic mean of these two readings is **719.0 RPM**.

This is an estimate based on the two endpoint measurements. It is not a
time-weighted measurement of the drum's average speed throughout the
entire ON/OFF cycle.

The difference between the two readings is approximately **21.5 RPM**,
or about **2.9%** of the speed measured at the end of the ON period.

## Summary

  Operating condition                                          Estimated speed
  ------------------------------------------------------ ---------------------
  Maximum speed, motor continuously on                             **768 RPM**
  Estimated mean, 4 s ON / 4 s OFF                                 **719 RPM**
  End of 4-second ON period                                          729.7 RPM
  End of 4-second OFF period                                         708.2 RPM
  Approximate onset of noticeable resonance                       200--225 RPM
  Region of strongest observed resonance                        Around 300 RPM
  Approximate region where vibration starts decreasing            343--379 RPM
  Temporarily stable region after resonance                Around 423--424 RPM

## Limitations

1.  The acceleration-ramp table is based on the cut positions in the
    Kdenlive project, not on an automated frame-by-frame tracking
    measurement.
2.  The project timeline is configured at 30 FPS, so cut placement has a
    nominal resolution of 1/30 second. The original 120 FPS footage may
    contain finer temporal information that is not represented by these
    timeline cut positions.
3.  The black reference mark may be slightly before or after the chosen
    reference position at each cut. This introduces uncertainty into
    individual revolution times.
4.  Resonance onset, peak intensity, and the point where vibration
    decreases are visual observations, not exact experimentally
    determined thresholds.
5.  The 719 RPM ON/OFF figure is the arithmetic mean of two endpoint
    readings, not a continuous average over the full operating cycle.
