#pragma once

#include <stdint.h>

namespace domain {

/**
 * @brief Water level states inside the washing machine tub.
 */
enum class WaterLevel : uint8_t {
    EMPTY = 0,
    LOW_LEVEL,
    MEDIUM_LEVEL,
    HIGH_LEVEL
};

/**
 * @brief Selectable wash programs.
 */
enum class WashProgram : uint8_t {
    NORMAL_WASH = 0, // Normal wash (agitation without soak)
    HEAVY_WASH,      // Heavy wash (agitation + long soak)
    RINSE_ONLY,      // Rinse only
    SPIN_ONLY        // Spin only
};

/**
 * @brief Operational washing stages during cycle execution and visual feedback.
 */
enum class WashStage : uint8_t {
    IDLE = 0,
    WASH,
    RINSE,
    SPIN
};

/**
 * @brief Machine macro operational states.
 */
enum class MachineState : uint8_t {
    IDLE = 0,
    RUNNING,
    PAUSED,
    ERROR,
    FINISHED
};

/**
 * @brief Machine fault and timeout error classifications.
 */
enum class MachineError : uint8_t {
    NONE = 0,
    FILL_TIMEOUT,    // Water inlet / solenoid valve timeout (legacy: type 1)
    DRAIN_TIMEOUT,   // Drain pump / water evacuation timeout (legacy: type 2)
    UNBALANCED_LOAD  // Critical tub vibration / unbalance trip (safety)
};

/**
 * @brief Discrete operational steps in Field Service / Diagnostic Mode.
 */
enum class DiagnosticStep : uint8_t {
    LEVEL_SENSOR = 0,    // Real-time pressure switch inspection
    VIBRATION_SENSOR,    // Real-time MPU-6050 vibration telemetry
    MAIN_VALVE,          // Solenoids 1 & 2 (Main water inlet valve)
    SOFTENER_VALVE,      // Solenoid 3 (Softener dispenser valve)
    DRAIN_PUMP,          // Drain pump and brake clutch actuator
    MOTOR_AGITATE,       // Agitation motor alternating CW/CCW
    SPIN_TEST,           // Centrifugal spin test with clutch and vibration guard
    COUNT
};

} // namespace domain
