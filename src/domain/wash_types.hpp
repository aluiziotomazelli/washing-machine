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

} // namespace domain
