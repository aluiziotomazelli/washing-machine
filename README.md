# 🧺 Washing Machine Custom Controller: From Spaghetti to Clean C++

> A practical embedded software engineering case study: restoring an old washing machine and refactoring legacy monolithic Arduino firmware into a modern, modular, non-blocking C++ architecture with native PC unit testing (Host Tests).

---

## 📖 About the Project

This project originates from the restoration of an old domestic washing machine whose original electronic control board failed. The original controller was replaced with an ATmega328P microcontroller board (Arduino Pro Mini / Nano).

The original hardware restoration and history were detailed in the 2017 Medium article (in Portuguese):
🔗 **[Controlando uma Lavadora de Roupas com Arduino (Medium, 2017)](https://medium.com/@aluiziotomazelli/controlando-uma-lavadora-de-roupas-com-arduino-b4aeb57a2abd)**

The initial baseline firmware (**v0.1.0**) successfully ran the wash cycles, but exhibited common limitations of quick Arduino prototypes:
- Monolithic code inside a single `.ino` file.
- Heavy reliance on blocking `while()` loops and `delay()` calls.
- Shared global state without encapsulation or protection.
- Inability to read real-time sensors (such as an I2C vibration accelerometer) during cycle execution.
- Absence of automated tests (every firmware change required testing directly on physical hardware).

This repository documents the step-by-step **architectural refactoring** of this firmware into a production-grade embedded C++ codebase.

📚 **Read the full engineering story and architectural breakdown in [`docs/case-study.md`](docs/case-study.md).**

---

## 🗺️ Evolution Roadmap & Releases

The project's architectural evolution is structured into milestones with dedicated Git tags and releases:

| Version | Milestone | Description |
| :---: | :--- | :--- |
| **`v0.1.0`** | 🍝 **Legacy Spaghetti (Baseline)** | Original monolithic `.ino` firmware, blocking delays, and global state. |
| **`v0.2.0`** | 🔌 **HAL & Host Unit Testing (Linux/PC)** | Pure C++ interfaces (`IGpioHAL`, `ITimerHAL`, `IButton`, `IDigitalOutput`, `IReversibleMotor`, `IWaterLevelSensor`, `IBuzzer`, `ILedPanel`), safety interlocks (motor dead-time), and automated Dual-Target Unit Testing (GoogleTest & GoogleMock on PC with CI). |
| **`v0.3.0`** | ⚙️ **Non-Blocking Finite State Machine** | Event-driven washing machine cycle coordinator powered by non-blocking ticks, eliminating all `delay()` and blocking loops. |
| **`v0.4.0`** | 🚨 **Sensors & Safety Watchdogs** | Water fill timeout protection (12-minute fail-safe), drain verification, and real-time I2C accelerometer vibration detection. |
| **`v0.5.0`** | 🌈 **Addressable RGB LEDs (WS2812B)** | Dynamic light animations (spinning chase, fluid level gauge, breathing fault alarms) via the `ILedPanel` interface. |
| **`v1.0.0`** | 🚀 **Production Modern C++** | Robust, fully documented, clean C++ firmware ready for deployment (including ESP32-C3 portability). |

---

## 🛠️ How to Build and Test

The project is structured to be **100% accessible to the Arduino community** without requiring proprietary tools or complex toolchains.

### 1. In Arduino IDE (Graphical Interface)
1. Open the [`washing-machine.ino`](washing-machine.ino) sketch in the Arduino IDE.
2. Select your board (**Arduino Pro or Pro Mini** / **Arduino Nano** - ATmega328P, 5V, 16MHz).
3. Click **Verify** / **Upload**.

### 2. Via Terminal / VS Code / Antigravity (`arduino-cli` and `Makefile`)
On Linux / macOS:

```bash
# Build the Arduino firmware (verbose output):
make build

# Flash firmware to the board via USB serial (e.g. FTDI):
make flash PORT=/dev/ttyUSB1

# Run the native Host Unit Tests on your PC (using GoogleTest / GoogleMock):
make test

# Generate compilation database for IDE IntelliSense:
make compile-db
```

---

## 📸 Schematics and Hardware Photos

Refer to the [`docs/`](docs/) directory for electrical schematics, pressure switch wiring diagrams, and photos of the assembled power/relay control board.
