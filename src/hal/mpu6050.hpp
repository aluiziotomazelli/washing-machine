#pragma once

#include "interfaces/i_accelerometer.hpp"
#include "interfaces/i_i2c_hal.hpp"

namespace hal {

/**
 * @enum AccelScale
 * @brief MPU-6050 accelerometer full scale range options.
 */
enum class AccelScale : uint8_t {
    SCALE_2G  = 0x00, // ±2g (16384 LSB/g)
    SCALE_4G  = 0x08, // ±4g (8192 LSB/g)
    SCALE_8G  = 0x10, // ±8g (4096 LSB/g)
    SCALE_16G = 0x18  // ±16g (2048 LSB/g)
};

/**
 * @class Mpu6050
 * @brief High-efficiency, ultra-lightweight driver for MPU-6050 accelerometer.
 *
 * Implements IAccelerometer over II2cHAL with zero dynamic allocations,
 * built-in WHO_AM_I device verification, and low-pass filtering.
 */
class Mpu6050 : public IAccelerometer {
public:
    static constexpr uint8_t k_default_address = 0x68;

    // MPU-6050 Register Map:
    static constexpr uint8_t k_reg_config       = 0x1A;
    static constexpr uint8_t k_reg_accel_config = 0x1C;
    static constexpr uint8_t k_reg_accel_xout_h = 0x3B;
    static constexpr uint8_t k_reg_pwr_mgmt_1   = 0x6B;
    static constexpr uint8_t k_reg_who_am_i     = 0x75;

    explicit Mpu6050(II2cHAL& i2c, uint8_t dev_addr = k_default_address, AccelScale scale = AccelScale::SCALE_4G);
    ~Mpu6050() override = default;

    bool init() override;
    bool read_accel(Vector3& accel) override;

    uint8_t get_device_address() const { return dev_addr_; }
    AccelScale get_scale() const { return scale_; }

private:
    II2cHAL& i2c_;
    uint8_t dev_addr_;
    AccelScale scale_;
};

} // namespace hal
