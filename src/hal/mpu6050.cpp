#include "mpu6050.hpp"

namespace hal {

Mpu6050::Mpu6050(II2cHAL& i2c, uint8_t dev_addr, AccelScale scale)
    : i2c_(i2c), dev_addr_(dev_addr), scale_(scale)
{
}

bool Mpu6050::init()
{
    i2c_.init(100000);

    // 1. Verify device signature via WHO_AM_I register (try primary address, then alternate)
    uint8_t addresses[2] = {dev_addr_, static_cast<uint8_t>((dev_addr_ == 0x68) ? 0x69 : 0x68)};
    bool found = false;

    for (uint8_t addr : addresses) {
        uint8_t who_am_i = 0;
        if (i2c_.read_bytes(addr, k_reg_who_am_i, &who_am_i, 1)) {
            // MPU-6050: 0x68, MPU-6500: 0x70, MPU-9250: 0x71, clone families: 0x72, 0x73, or 6-bit mask
            if (who_am_i == 0x68 || who_am_i == 0x70 || who_am_i == 0x71 || who_am_i == 0x72 || who_am_i == 0x73 || (who_am_i & 0x7E) == 0x68) {
                dev_addr_ = addr;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        is_initialized_ = false;
        return false;
    }

    // 2. Wake up device (clear SLEEP bit in PWR_MGMT_1)
    if (!i2c_.write_reg(dev_addr_, k_reg_pwr_mgmt_1, 0x00)) {
        is_initialized_ = false;
        return false;
    }

    // 3. Configure Digital Low Pass Filter (DLPF_CFG = 3 -> ~42 Hz bandwidth)
    // Filters out high-frequency motor vibration and electrical noise
    if (!i2c_.write_reg(dev_addr_, k_reg_config, 0x03)) {
        is_initialized_ = false;
        return false;
    }

    // 4. Configure full-scale accelerometer range (default: ±4g)
    if (!i2c_.write_reg(dev_addr_, k_reg_accel_config, static_cast<uint8_t>(scale_))) {
        is_initialized_ = false;
        return false;
    }

    is_initialized_ = true;
    return true;
}

bool Mpu6050::read_accel(Vector3& accel)
{
    if (!is_initialized_) {
        if (!init()) {
            return false;
        }
    }

    uint8_t raw[6];
    if (!i2c_.read_bytes(dev_addr_, k_reg_accel_xout_h, raw, 6)) {
        is_initialized_ = false;
        return false;
    }

    // Assemble high and low bytes (big-endian from MPU-6050 registers)
    accel.x = static_cast<int16_t>((static_cast<uint16_t>(raw[0]) << 8) | raw[1]);
    accel.y = static_cast<int16_t>((static_cast<uint16_t>(raw[2]) << 8) | raw[3]);
    accel.z = static_cast<int16_t>((static_cast<uint16_t>(raw[4]) << 8) | raw[5]);

    return true;
}

} // namespace hal
