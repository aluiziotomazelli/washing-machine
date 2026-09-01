# ==============================================================================
# Washing Machine Firmware & Host Test Makefile
# ==============================================================================

FQBN ?= arduino:avr:pro:cpu=16MHzatmega328
PORT ?= /dev/ttyUSB1
BUILD_DIR = build
LIBS_DIR = libraries
SKETCH = washing-machine.ino

# Host Compiler & GoogleTest / GoogleMock settings
HOST_CXX = g++
GOOGLETEST_DIR = test/lib/googletest
GTEST_DIR = $(GOOGLETEST_DIR)/googletest
GMOCK_DIR = $(GOOGLETEST_DIR)/googlemock

GMOCK_INC = -I$(GTEST_DIR)/include -I$(GTEST_DIR) -I$(GMOCK_DIR)/include -I$(GMOCK_DIR)
GMOCK_SRC = $(GTEST_DIR)/src/gtest-all.cc $(GMOCK_DIR)/src/gmock-all.cc $(GMOCK_DIR)/src/gmock_main.cc

HOST_CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -Isrc -Itest $(GMOCK_INC) -DHOST_TEST
TEST_SRC = $(wildcard test/*.cpp) $(shell find src -name "*.cpp" ! -path "*/arduino/*")
TEST_BIN = $(BUILD_DIR)/test_runner

.PHONY: all build flash test clean compile-db setup-gtest

all: build

# ------------------------------------------------------------------------------
# Arduino Compilation & Flash (via arduino-cli)
# ------------------------------------------------------------------------------
build:
	@mkdir -p $(BUILD_DIR)
	@echo "==> Compiling Arduino firmware ($(FQBN)) with verbose output..."
	arduino-cli compile --fqbn $(FQBN) --libraries $(LIBS_DIR) --build-path $(BUILD_DIR) -v --warnings all .
	@echo "==> Build finished successfully."

flash:
	@echo "==> Uploading firmware to $(PORT)..."
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) --input-dir $(BUILD_DIR) -v .

compile-db:
	@mkdir -p $(BUILD_DIR)
	arduino-cli compile --fqbn $(FQBN) --libraries $(LIBS_DIR) --build-path $(BUILD_DIR) --only-compilation-database .

# ------------------------------------------------------------------------------
# Native Host Unit Tests (GoogleTest & GoogleMock)
# ------------------------------------------------------------------------------
setup-gtest: $(GOOGLETEST_DIR)

$(GOOGLETEST_DIR):
	@mkdir -p test/lib
	@echo "==> Downloading GoogleTest / GoogleMock (v1.14.0) into test/lib/..."
	@curl -fsSL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz | tar -xz -C test/lib
	@mv test/lib/googletest-1.14.0 $(GOOGLETEST_DIR)
	@echo "==> GoogleTest & GoogleMock setup complete."

test: $(GOOGLETEST_DIR)
	@mkdir -p $(BUILD_DIR)
	@echo "==> Compiling Host Unit Tests with GoogleTest & GoogleMock..."
	$(HOST_CXX) $(HOST_CXXFLAGS) $(GMOCK_SRC) $(TEST_SRC) -o $(TEST_BIN)
	@echo "==> Running Host Unit Tests..."
	@./$(TEST_BIN)

clean:
	@echo "==> Cleaning build directory..."
	@rm -rf $(BUILD_DIR) compile_commands.json
