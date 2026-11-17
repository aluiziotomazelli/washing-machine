# Washing-machine drum — acceleration ramp and resonance band

Analysis of the cut timings in `rpm-analysis].kdenlive` to derive the drum
speed (RPM) over time and locate the resonance range where the drum starts
shaking heavily.

## Method and assumptions

- **Source of truth:** `playlist2` in the Kdenlive project. It contains 33
  consecutive segments of `YouCut_20261116_142323061.mp4` (video `0 → 31.5 s`).
  Each segment boundary is one of the user's cuts, and **each cut corresponds
  to one full drum revolution** (a mark on the drum passing the same point).
- **Slow motion factor 4×:** captured at 120 fps, edited on a 30 fps timeline,
  so the video runs 4× slower than reality:
  `t_real = t_video / 4`.
- **Revolution period** = time between two consecutive cuts.
- **RPM formula:**

  ```
  RPM = 60 / (Δt_video / 4) = 240 / Δt_video = 7200 / Δframes
  ```

- **Timestamps** below use `SS:FF` (seconds:frames at 30 fps), e.g.
  `14:29 = 14 s + 29 frames = 14.967 s`.
- The 4× factor is independently validated: a separate 30-revolution
  measurement gave **~768 RPM** at cruise speed, against **750 RPM** in the
  machine manual (+2.4%).

## Acceleration ramp (one sample per revolution)

| Rev | Start (video `SS:FF`) | Start video (s) | Start real (s) | Period video (s) | Period real (s) | RPM | Accel (RPM/s)* |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 00:00 | 0.00 | 0.00 | 6.100 | 1.525 | 39 | — |
| 2 | 06:03 | 6.10 | 1.52 | 2.833 | 0.708 | 85 | 41 |
| 3 | 08:28 | 8.93 | 2.23 | 1.967 | 0.492 | 122 | 62 |
| 4 | 10:27 | 10.90 | 2.73 | 1.667 | 0.417 | 144 | 48 |
| 5 | 12:17 | 12.57 | 3.14 | 1.366 | 0.341 | 176 | 84 |
| 6 | 13:28 | 13.93 | 3.48 | 1.200 | 0.300 | 200 | 76 |
| 7 | 15:04 | 15.13 | 3.78 | 1.067 | 0.267 | 225 | 88 |
| 8 | 16:06 | 16.20 | 4.05 | 1.000 | 0.250 | 240 | 58 |
| 9 | 17:06 | 17.20 | 4.30 | 0.900 | 0.225 | 267 | 112 |
| 10 | 18:03 | 18.10 | 4.53 | 0.867 | 0.217 | 277 | 46 |
| 11 | 18:29 | 18.97 | 4.74 | 0.833 | 0.208 | 288 | 53 |
| 12 | 19:24 | 19.80 | 4.95 | 0.733 | 0.183 | 327 | 201 |
| 13 | 20:16 | 20.53 | 5.13 | 0.700 | 0.175 | 343 | 86 |
| 14 | 21:07 | 21.23 | 5.31 | 0.734 | 0.183 | 327 | −89 |
| 15 | 21:29 | 21.97 | 5.49 | 0.700 | 0.175 | 343 | 89 |
| 16 | 22:20 | 22.67 | 5.67 | 0.633 | 0.158 | 379 | 218 |
| 17 | 23:09 | 23.30 | 5.83 | 0.633 | 0.158 | 379 | 0 |
| 18 | 23:28 | 23.93 | 5.98 | 0.567 | 0.142 | 423 | 294 |
| 19 | 24:15 | 24.50 | 6.12 | 0.567 | 0.142 | 423 | 0 |
| 20 | 25:02 | 25.07 | 6.27 | 0.566 | 0.141 | 424 | 5 |
| 21 | 25:19 | 25.63 | 6.41 | 0.534 | 0.134 | 449 | 185 |
| 22 | 26:05 | 26.17 | 6.54 | 0.500 | 0.125 | 480 | 236 |
| 23 | 26:20 | 26.67 | 6.67 | 0.500 | 0.125 | 480 | 0 |
| 24 | 27:05 | 27.17 | 6.79 | 0.500 | 0.125 | 480 | 0 |
| 25 | 27:20 | 27.67 | 6.92 | 0.466 | 0.116 | 515 | 290 |
| 26 | 28:04 | 28.13 | 7.03 | 0.467 | 0.117 | 514 | −9 |
| 27 | 28:18 | 28.60 | 7.15 | 0.433 | 0.108 | 554 | 359 |
| 28 | 29:01 | 29.03 | 7.26 | 0.434 | 0.108 | 553 | −12 |
| 29 | 29:14 | 29.47 | 7.37 | 0.433 | 0.108 | 554 | 12 |
| 30 | 29:27 | 29.90 | 7.47 | 0.433 | 0.108 | 554 | 0 |
| 31 | 30:10 | 30.33 | 7.58 | 0.400 | 0.100 | 600 | 439 |
| 32 | 30:22 | 30.73 | 7.68 | 0.400 | 0.100 | 600 | 0 |
| 33 | 31:04 | 31.13 | 7.78 | 0.367 | 0.092 | 654 | 563 |

\*Acceleration is estimated between revolution midpoints and is only
indicative: with one sample per revolution, frame quantization (±1 frame
≈ ±8–12 RPM near 300 RPM) produces large jumps and occasional negative
values.

**Overall ramp:** ~39 RPM → ~654 RPM, a change of ~615 RPM over ~7.9 s of
real time (average ~78 RPM/s, not linear). See `rpm_ramp.png`.

## Resonance band

Using the user's observed window (`14:29 → 23:03`, peak at `19:12`):

| Event | Video time | Real time | Measured RPM |
|---|---|---:|---:|
| Shaking starts | 14:29 (14.967 s) | 3.74 s | **~200** |
| Peak | 19:12 (19.400 s) | 4.85 s | **~288** |
| Shaking stops | 23:03 (23.100 s) | 5.78 s | **~379** |

- **Measured resonance band:** **~200–380 RPM**.
- **Core / strongest band:** **~240–380 RPM**.
- **Peak intensity:** **~285–300 RPM** (the 288 RPM sample at `19:12`,
  continuing into a ~300 RPM plateau).
- **Signature of wobble:** rev 14 (`21:07–21:29`) stretches from 343 to
  327 RPM (≈ −89 RPM/s) before recovering — consistent with the drum
  binding/swinging through the critical speed.

## Cross-check and consistency

- Cruise speed measured separately: 30 revolutions in `09:11`
  (9.367 s video = 2.342 s real) → **~768 RPM**, against **750 RPM** from the
  manual (**+2.4%**). This confirms the 4× slow-motion assumption; without it
  the result would be ~192 RPM, far from the manual.
- The older `drum_rpm_measurements_en.md` reaches the same conclusions
  (onset ~200–218 RPM, peak ~300 RPM, decay from ~360 RPM) using manually
  noted timestamps. Those notes are offset by ~2–7 frames relative to the
  exact `.kdenlive` cut positions used here.

## Limitations

1. **One measurement per revolution.** RPM is assumed constant within each
   interval; real acceleration between cuts is not resolved.
2. **Frame quantization.** ±1 frame at 30 fps is ±0.033 s of video
   (±0.0083 s real), i.e. roughly ±8–12 RPM around 300 RPM.
3. **Manual cut placement.** The mark-passing instants depend on how precisely
   each cut was placed; small offsets accumulate into the period estimates.
4. **Resonance limits are observational.** The `14:29 → 23:03` window is a
   perceptual estimate of when shaking is strong, not a controlled sweep.

## Summary

| Quantity | Value |
|---|---|
| Revolutions measured | 33 |
| Video span / real span | 31.5 s / 7.9 s |
| Speed range in this clip | ~39 → ~654 RPM |
| Resonance onset | ~200 RPM (14:29) |
| Resonance peak | ~285–300 RPM (19:12) |
| Resonance end | ~379 RPM (23:03) |
| Resonance band | **~200–380 RPM** (core ~240–380) |
| Cruise speed (separate measurement) | ~768 RPM (manual: 750 RPM) |

## Files

- `rpm_ramp.csv` — full per-revolution data.
- `rpm_ramp.png` — acceleration ramp with the resonance band highlighted.
