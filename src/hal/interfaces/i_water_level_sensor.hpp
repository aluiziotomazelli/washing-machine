#pragma once

#include <stdint.h>

namespace hal {

/**
 * @brief Normalized water levels in the washing machine tub.
 */
enum class WaterLevel : uint8_t {
    EMPTY = 0,
    LOW_LEVEL,
    MEDIUM_LEVEL,
    HIGH_LEVEL
};

/**
 * @interface IWaterLevelSensor
 * @brief Abstract interface for reading tub water level from mechanical pressure switches or pressure transducers.
 */
class IWaterLevelSensor {
public:
    virtual ~IWaterLevelSensor() = default;

    /**
     * @brief Initialize hardware pins and internal debouncer states.
     */
    virtual void init() = 0;

    /**
     * @brief Periodic non-blocking sensor update (poll inputs and update debouncing).
     */
    virtual void update() = 0;

    /**
     * @brief Check whether water has reached or exceeded a target level.
     * @param target Desired target level (LOW_LEVEL, MEDIUM_LEVEL, HIGH_LEVEL).
     * @return true if target level has been reached, false otherwise.
     */
    virtual bool is_level_reached(WaterLevel target) const = 0;

    /**
     * @brief Query the highest water level currently satisfied in the tub.
     * @return WaterLevel Current measured level.
     */
    virtual WaterLevel get_current_level() const = 0;

    /**
     * @brief Check whether the tub is empty (below the lowest detected level).
     * @return true if empty, false if water is present.
     */
    virtual bool is_empty() const = 0;
};

} // namespace hal
