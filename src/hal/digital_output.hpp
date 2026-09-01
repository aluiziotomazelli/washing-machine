#pragma once

#include <stdint.h>
#include "interfaces/i_digital_output.hpp"
#include "interfaces/i_gpio_hal.hpp"

namespace hal {

/**
 * @class DigitalOutput
 * @brief Platform-agnostic digital output controller for relays, valves, and indicators.
 * 
 * Uses an intrusive static linked list to allow zero-overhead, heap-free tracking of all
 * instantiated outputs and global batch operations (such as emergency shut-off via turn_off_all()).
 */
class DigitalOutput : public IDigitalOutput {
public:
    /**
     * @brief Construct a new DigitalOutput and register it into the static linked list.
     * @param gpio_hal Reference to the GPIO hardware abstraction layer.
     * @param pin Hardware pin number.
     * @param active_low Set true if the relay/load activates on LOW level.
     * @param initial_state Initial state (true = ON, false = OFF).
     */
    DigitalOutput(
        IGpioHAL& gpio_hal,
        uint8_t pin,
        bool active_low = false,
        bool initial_state = false
    );

    ~DigitalOutput() override = default;

    void init() override;
    void turn_on() override;
    void turn_off() override;
    void toggle() override;
    bool is_on() const override;
    uint8_t get_pin() const override;

    /**
     * @brief Batch initialize all instantiated DigitalOutput objects.
     */
    static void init_all();

    /**
     * @brief Emergency / batch turn-off for all registered DigitalOutput objects.
     */
    static void turn_off_all();

    /**
     * @brief Reset the internal static registry (primarily for test fixture teardown).
     */
    static void reset_registry();

private:
    IGpioHAL& gpio_hal_;
    uint8_t pin_;
    bool active_low_;
    bool is_on_;
    bool is_initialized_{false};

    void apply_hardware_level();

    // Intrusive linked list tracking
    static DigitalOutput* head_;
    DigitalOutput* next_{nullptr};
};

} // namespace hal
