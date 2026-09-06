#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_eeprom_hal.hpp"

class EepromHalTest : public ::testing::Test {
protected:
    mocks::MockEepromHAL mock_eeprom;
};

TEST_F(EepromHalTest, InitializesWithFactoryErasedStateAndFullCapacity)
{
    EXPECT_EQ(mock_eeprom.capacity(), 1024u);
    EXPECT_EQ(mock_eeprom.read_byte(0), 0xFF);
    EXPECT_EQ(mock_eeprom.read_byte(512), 0xFF);
    EXPECT_EQ(mock_eeprom.read_byte(1023), 0xFF);
    EXPECT_EQ(mock_eeprom.get_write_count(), 0u);
}

TEST_F(EepromHalTest, WritesAndReadsSingleByteCorrectly)
{
    mock_eeprom.write_byte(10, 0x42);
    EXPECT_EQ(mock_eeprom.read_byte(10), 0x42);
    EXPECT_EQ(mock_eeprom.get_write_count(), 1u);
}

TEST_F(EepromHalTest, UpdateSemanticsOnlyIncrementsWriteCountWhenByteChanges)
{
    // First write: changes 0xFF to 0x55 -> 1 write
    mock_eeprom.write_byte(20, 0x55);
    EXPECT_EQ(mock_eeprom.get_write_count(), 1u);

    // Second write: writes identical value 0x55 -> 0 writes!
    mock_eeprom.write_byte(20, 0x55);
    EXPECT_EQ(mock_eeprom.get_write_count(), 1u);

    // Third write: changes 0x55 to 0xAA -> 1 new write (total 2)
    mock_eeprom.write_byte(20, 0xAA);
    EXPECT_EQ(mock_eeprom.get_write_count(), 2u);
}

TEST_F(EepromHalTest, ReadsAndWritesBlockCorrectly)
{
    uint8_t payload[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    mock_eeprom.write_block(100, payload, sizeof(payload));

    EXPECT_EQ(mock_eeprom.get_write_count(), 8u);

    uint8_t readback[8] = {0};
    mock_eeprom.read_block(100, readback, sizeof(readback));

    EXPECT_EQ(memcmp(payload, readback, sizeof(payload)), 0);
}

TEST_F(EepromHalTest, BlockUpdateOnlyWritesModifiedBytes)
{
    uint8_t initial[4] = {0x10, 0x20, 0x30, 0x40};
    mock_eeprom.write_block(50, initial, sizeof(initial));
    EXPECT_EQ(mock_eeprom.get_write_count(), 4u);

    // Modify only index 1 and 3
    uint8_t updated[4] = {0x10, 0x99, 0x30, 0x88};
    mock_eeprom.write_block(50, updated, sizeof(updated));

    // Only 2 bytes changed, so write_count should increase by 2 (total 6)
    EXPECT_EQ(mock_eeprom.get_write_count(), 6u);
}

TEST_F(EepromHalTest, HandlesOutOfBoundsAccessSafely)
{
    EXPECT_EQ(mock_eeprom.read_byte(1024), 0xFF);
    EXPECT_EQ(mock_eeprom.read_byte(2000), 0xFF);

    // Write out of bounds should be ignored safely
    mock_eeprom.write_byte(1024, 0x12);
    EXPECT_EQ(mock_eeprom.get_write_count(), 0u);
}
