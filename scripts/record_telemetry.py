#!/usr/bin/env python3
"""
Record telemetry from Arduino MPU-6050 accelerometer to CSV file.
Designed for washing machine spin vibration analysis.
"""

import sys
import os
import time
import re
import argparse
import serial

DEFAULT_PORT = "/dev/ttyUSB1"
if os.path.exists("/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A9M9DV3R-if00-port0"):
    DEFAULT_PORT = os.path.realpath("/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A9M9DV3R-if00-port0")

PATTERN = re.compile(r"X:([-\d]+)\s+Y:([-\d]+)\s+Z:([-\d]+)\s+Vib:([-\d]+)")

def main():
    parser = argparse.ArgumentParser(description="Record MPU-6050 telemetry to a CSV file.")
    parser.add_argument("-p", "--port", default=DEFAULT_PORT, help=f"Serial port (default: {DEFAULT_PORT})")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("-d", "--duration", type=float, default=50.0, help="Recording duration in seconds (default: 50s)")
    parser.add_argument("-o", "--output", default=None, help="Path to output CSV file")
    args = parser.parse_args()

    os.makedirs("logs", exist_ok=True)
    if args.output is None:
        timestamp_str = time.strftime("%Y%m%d_%H%M%S")
        args.output = f"logs/spin_{timestamp_str}.csv"

    print("=" * 70)
    print(" WASHING MACHINE: MPU-6050 TELEMETRY RECORDER")
    print("=" * 70)
    print(f"Serial port : {args.port}")
    print(f"Baud rate   : {args.baud}")
    print(f"Duration    : {args.duration:.1f} seconds")
    print(f"CSV output  : {args.output}")
    print("-" * 70)

    try:
        ser = serial.Serial()
        ser.port = args.port
        ser.baudrate = args.baud
        ser.timeout = 1.0
        ser.dtr = False
        ser.rts = False
        ser.open()
    except serial.SerialException as e:
        print(f"ERROR: Could not open serial port {args.port}: {e}")
        sys.exit(1)

    time.sleep(0.5)
    ser.reset_input_buffer()

    print("Synchronizing with MPU-6050 stream...")
    samples = []
    max_vib = 0
    min_vib = 999999
    sum_vib = 0

    start_time = None

    try:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write("elapsed_ms,elapsed_s,x,y,z,vib\n")

            while True:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                match = PATTERN.search(line)
                if not match:
                    continue

                now = time.time()
                if start_time is None:
                    start_time = now
                    print(f"Recording started ({args.duration:.0f}s - press Ctrl+C to abort early)")

                elapsed_s = now - start_time
                elapsed_ms = int(elapsed_s * 1000)

                x = int(match.group(1))
                y = int(match.group(2))
                z = int(match.group(3))
                vib = int(match.group(4))

                f.write(f"{elapsed_ms},{elapsed_s:.3f},{x},{y},{z},{vib}\n")
                samples.append(vib)

                if vib > max_vib:
                    max_vib = vib
                if vib < min_vib:
                    min_vib = vib
                sum_vib += vib

                progress = min(1.0, elapsed_s / args.duration)
                bar_len = 25
                filled = int(bar_len * progress)
                bar = "#" * filled + "-" * (bar_len - filled)

                sys.stdout.write(
                    f"\r[{bar}] {elapsed_s:4.1f}s / {args.duration:4.1f}s | "
                    f"Samples: {len(samples):4d} | Current Vib: {vib:5d} | Max Vib: {max_vib:5d}"
                )
                sys.stdout.flush()

                if elapsed_s >= args.duration:
                    break

    except KeyboardInterrupt:
        print("\n\nRecording aborted by user via Ctrl+C.")
    finally:
        ser.close()

    print("\n" + "=" * 70)
    print(" CAPTURE SUMMARY")
    print("=" * 70)
    if samples:
        actual_duration = elapsed_s if start_time else 0
        avg_vib = sum_vib / len(samples)
        hz = len(samples) / actual_duration if actual_duration > 0 else 0
        print(f"Total samples saved   : {len(samples)}")
        print(f"Average sampling rate : {hz:.1f} Hz")
        print(f"Minimum vibration     : {min_vib}")
        print(f"Average vibration     : {avg_vib:.1f}")
        print(f"Maximum vibration     : {max_vib}")
        print(f"CSV file saved to     : {args.output}")
    else:
        print("No samples collected.")
    print("=" * 70)

if __name__ == "__main__":
    main()
