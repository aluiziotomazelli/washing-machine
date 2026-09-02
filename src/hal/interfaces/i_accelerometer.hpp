#pragma once

#include <stdint.h>

namespace hal {

/**
 * @struct Vector3
 * @brief 3-axis raw sensor reading vector (X, Y, Z).
 */
struct Vector3 {
    int16_t x{0};
    int16_t y{0};
    int16_t z{0};
};

/**
 * @interface IAccelerometer
 * @brief Hardware Abstraction Layer interface for 3-axis accelerometers.
 */
class IAccelerometer {
public:
    virtual ~IAccelerometer() = default;

    /**
     * @brief Initialize and verify communication with the sensor.
     * @return true if sensor responded and configured successfully, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief Read 3-axis acceleration sample.
     * @param accel Output vector reference to populate on success.
     * @return true if read was completely successful, false on bus error or timeout.
     */
    virtual bool read_accel(Vector3& accel) = 0;
};

} // namespace hal
