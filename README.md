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
| **`v0.2.0`** | 🔌 **Hardware Abstraction Layer (HAL)** | Hardware isolation via pure C++ interfaces (`IActuators`, `ISensors`) and safety locks (motor *dead-time* preventing reversal short circuits). |
| **`v0.3.0`** | ⚙️ **Non-Blocking FSM** | Event-driven Finite State Machine powered by `millis()`, eliminating all `delay()` and blocking loops. |
| **`v0.4.0`** | 🧪 **Host Unit Testing (Linux/PC)** | Automated unit test suite using *Mocks* compiled natively with `g++`, validating cycles and timeout fail-safes in milliseconds. |
| **`v0.5.0`** | 🚨 **I2C Watchdog & WS2812B** | Real-time out-of-balance detection via I2C accelerometer and migration of 7 indicator LEDs to a single addressable RGB LED line. |
| **`v1.0.0`** | 🚀 **Production Modern C++** | Robust, fully documented, clean C++ firmware ready for deployment. |

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
