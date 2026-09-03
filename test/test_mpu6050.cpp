#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "hal/mpu6050.hpp"
#include "mocks/mock_i2c_hal.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArrayArgument;

class Mpu6050Test : public ::testing::Test {
protected:
    NiceMock<mocks::MockI2cHAL> mock_i2c;
    hal::Mpu6050 mpu{mock_i2c, 0x68, hal::AccelScale::SCALE_4G};
};

TEST_F(Mpu6050Test, InitializesSuccessfullyWhenWhoAmIMatchesAndWritesRegisters)
{
    // WHO_AM_I returns 0x68
    uint8_t who_am_i_val = 0x68;
    EXPECT_CALL(mock_i2c, read_bytes(0x68, 0x75, _, 1))
        .WillOnce(DoAll(SetArrayArgument<2>(&who_am_i_val, &who_am_i_val + 1), Return(true)));

    // Wake up (PWR_MGMT_1 = 0)
    EXPECT_CALL(mock_i2c, write_reg(0x68, 0x6B, 0x00)).WillOnce(Return(true));

    // Low pass filter config (CONFIG = 3)
    EXPECT_CALL(mock_i2c, write_reg(0x68, 0x1A, 0x03)).WillOnce(Return(true));

    // Full scale config (ACCEL_CONFIG = 0x08 for ±4g)
    EXPECT_CALL(mock_i2c, write_reg(0x68, 0x1C, 0x08)).WillOnce(Return(true));

    EXPECT_TRUE(mpu.init());
}

TEST_F(Mpu6050Test, FailsInitializationWhenWhoAmIReturnsIncorrectDeviceSignature)
{
    // Returns 0x72 instead of 0x68
    uint8_t wrong_id = 0x72;
    EXPECT_CALL(mock_i2c, read_bytes(0x68, 0x75, _, 1))
        .WillOnce(DoAll(SetArrayArgument<2>(&wrong_id, &wrong_id + 1), Return(true)));

    EXPECT_FALSE(mpu.init());
}

TEST_F(Mpu6050Test, FailsInitializationWhenWakeupWriteFails)
{
    uint8_t who_am_i_val = 0x68;
    EXPECT_CALL(mock_i2c, read_bytes(0x68, 0x75, _, 1))
        .WillOnce(DoAll(SetArrayArgument<2>(&who_am_i_val, &who_am_i_val + 1), Return(true)));

    // Wake up fails on I2C bus
    EXPECT_CALL(mock_i2c, write_reg(0x68, 0x6B, 0x00)).WillOnce(Return(false));

    EXPECT_FALSE(mpu.init());
}

TEST_F(Mpu6050Test, ReadsAccelerationAndCorrectlyAssembles16BitSignedValues)
{
    // X = 0x0100 (256), Y = 0x0200 (512), Z = 0x2000 (8192, 1g at ±4g)
    uint8_t raw_data[6] = {0x01, 0x00, 0x02, 0x00, 0x20, 0x00};

    EXPECT_CALL(mock_i2c, read_bytes(0x68, 0x3B, _, 6))
        .WillOnce(DoAll(SetArrayArgument<2>(raw_data, raw_data + 6), Return(true)));

    hal::Vector3 accel;
    EXPECT_TRUE(mpu.read_accel(accel));

    EXPECT_EQ(accel.x, 256);
    EXPECT_EQ(accel.y, 512);
    EXPECT_EQ(accel.z, 8192);
}

TEST_F(Mpu6050Test, HandlesNegativeAccelerationValuesInTwoComplementCorrectly)
{
    // X = -1 (0xFFFF), Y = -500 (0xFE0C), Z = -8192 (0xE000)
    uint8_t raw_data[6] = {0xFF, 0xFF, 0xFE, 0x0C, 0xE0, 0x00};

    EXPECT_CALL(mock_i2c, read_bytes(0x68, 0x3B, _, 6))
        .WillOnce(DoAll(SetArrayArgument<2>(raw_data, raw_data + 6), Return(true)));

    hal::Vector3 accel;
    EXPECT_TRUE(mpu.read_accel(accel));

    EXPECT_EQ(accel.x, -1);
    EXPECT_EQ(accel.y, -500);
    EXPECT_EQ(accel.z, -8192);
}

TEST_F(Mpu6050Test, ReturnsFalseAndLeavesOutputVectorUntouchedOnI2cReadFailure)
{
    // I2C communication fails (NACK or timeout)
    EXPECT_CALL(mock_i2c, read_bytes(0x68, 0x3B, _, 6)).WillOnce(Return(false));

    hal::Vector3 accel{10, 20, 30};
    EXPECT_FALSE(mpu.read_accel(accel));

    // Vector must retain original values, NOT be corrupted or zeroed
    EXPECT_EQ(accel.x, 10);
    EXPECT_EQ(accel.y, 20);
    EXPECT_EQ(accel.z, 30);
}
