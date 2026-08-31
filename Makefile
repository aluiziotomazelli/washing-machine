# ==============================================================================
# Washing Machine Firmware & Host Test Makefile
# ==============================================================================

FQBN ?= arduino:avr:nano
PORT ?= /dev/ttyUSB0
BUILD_DIR = build
SKETCH = washing-machine.ino

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Isrc -Itest -DHOST_TEST

TEST_SRC = $(wildcard test/*.cpp) $(wildcard src/core/*.cpp)
TEST_BIN = $(BUILD_DIR)/test_runner

.PHONY: all build flash test clean

all: build

# ------------------------------------------------------------------------------
# Arduino Compilation & Flash (via arduino-cli)
# ------------------------------------------------------------------------------
build:
	@mkdir -p $(BUILD_DIR)
	@echo "==> Compiling Arduino firmware ($(FQBN))..."
	arduino-cli compile --fqbn $(FQBN) --output-dir $(BUILD_DIR) .
	@echo "==> Build finished successfully."

flash:
	@echo "==> Uploading firmware to $(PORT)..."
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) --input-dir $(BUILD_DIR) .

# ------------------------------------------------------------------------------
# Native Host Unit Tests (via g++)
# ------------------------------------------------------------------------------
test:
	@mkdir -p $(BUILD_DIR)
	@if [ -n "$(wildcard test/*.cpp)" ]; then \
		echo "==> Building and running Host Unit Tests (g++)..."; \
		$(CXX) $(CXXFLAGS) $(TEST_SRC) -o $(TEST_BIN); \
		./$(TEST_BIN); \
	else \
		echo "==> No host tests found in test/ yet. Create tests to run."; \
	fi

clean:
	@echo "==> Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
