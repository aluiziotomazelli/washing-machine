#include <gtest/gtest.h>
#include "persistence/cycle_persistence_manager.hpp"
#include "mocks/mock_eeprom_hal.hpp"

using namespace persistence;
using domain::WashProgram;
using domain::WaterLevel;

class CyclePersistenceManagerTest : public ::testing::Test {
protected:
    mocks::MockEepromHAL mock_eeprom;
};

TEST_F(CyclePersistenceManagerTest, HandlesFactoryErasedEepromCleanly)
{
    mock_eeprom.clear_all(0xFF);
    CyclePersistenceManager manager(mock_eeprom);

    CycleSnapshot snapshot{};
    bool found = manager.load_latest(snapshot);

    EXPECT_FALSE(found);
    EXPECT_FALSE(manager.has_valid_snapshot());
    EXPECT_EQ(manager.get_current_seq_num(), 0);
}

TEST_F(CyclePersistenceManagerTest, HandlesZeroFilledEepromCleanly)
{
    mock_eeprom.clear_all(0x00);
    CyclePersistenceManager manager(mock_eeprom);

    CycleSnapshot snapshot{};
    bool found = manager.load_latest(snapshot);

    EXPECT_FALSE(found);
    EXPECT_FALSE(manager.has_valid_snapshot());
    EXPECT_EQ(manager.get_current_seq_num(), 0);
}

TEST_F(CyclePersistenceManagerTest, SavesAndLoadsUserSelection)
{
    CyclePersistenceManager manager(mock_eeprom);

    manager.save_user_selection(WashProgram::HEAVY_WASH, WaterLevel::HIGH_LEVEL, true);

    EXPECT_TRUE(manager.has_valid_snapshot());
    EXPECT_EQ(manager.get_current_seq_num(), 1);

    // Simulate power cycle by creating a new persistence manager instance
    CyclePersistenceManager boot_manager(mock_eeprom);
    CycleSnapshot loaded{};
    bool found = boot_manager.load_latest(loaded);

    EXPECT_TRUE(found);
    EXPECT_EQ(loaded.seq_num, 1);
    EXPECT_EQ(loaded.program, WashProgram::HEAVY_WASH);
    EXPECT_EQ(loaded.level, WaterLevel::HIGH_LEVEL);
    EXPECT_TRUE(loaded.softener_enabled);
    EXPECT_FALSE(loaded.is_running);
    EXPECT_EQ(loaded.step_index, 0);
    EXPECT_FALSE(loaded.in_rinse_subcycle);
}

TEST_F(CyclePersistenceManagerTest, SavesAndLoadsCycleProgress)
{
    CyclePersistenceManager manager(mock_eeprom);

    manager.save_cycle_progress(
        WashProgram::NORMAL_WASH,
        WaterLevel::MEDIUM_LEVEL,
        true,
        3,
        true
    );

    CyclePersistenceManager reboot_manager(mock_eeprom);
    CycleSnapshot loaded{};
    bool found = reboot_manager.load_latest(loaded);

    EXPECT_TRUE(found);
    EXPECT_EQ(loaded.seq_num, 1);
    EXPECT_EQ(loaded.program, WashProgram::NORMAL_WASH);
    EXPECT_EQ(loaded.level, WaterLevel::MEDIUM_LEVEL);
    EXPECT_TRUE(loaded.softener_enabled);
    EXPECT_TRUE(loaded.is_running);
    EXPECT_FALSE(loaded.is_paused);
    EXPECT_EQ(loaded.step_index, 3);
    EXPECT_TRUE(loaded.in_rinse_subcycle);
}

TEST_F(CyclePersistenceManagerTest, SavesAndLoadsPausedCycleProgress)
{
    CyclePersistenceManager manager(mock_eeprom);

    manager.save_cycle_progress(
        WashProgram::HEAVY_WASH,
        WaterLevel::HIGH_LEVEL,
        true,
        2,
        false,
        true // is_paused
    );

    CyclePersistenceManager reboot_manager(mock_eeprom);
    CycleSnapshot loaded{};
    bool found = reboot_manager.load_latest(loaded);

    EXPECT_TRUE(found);
    EXPECT_EQ(loaded.seq_num, 1);
    EXPECT_EQ(loaded.program, WashProgram::HEAVY_WASH);
    EXPECT_EQ(loaded.level, WaterLevel::HIGH_LEVEL);
    EXPECT_TRUE(loaded.softener_enabled);
    EXPECT_TRUE(loaded.is_running);
    EXPECT_TRUE(loaded.is_paused);
    EXPECT_EQ(loaded.step_index, 2);
    EXPECT_FALSE(loaded.in_rinse_subcycle);
}

TEST_F(CyclePersistenceManagerTest, SavesCycleFinishedClearingRunningFlag)
{
    CyclePersistenceManager manager(mock_eeprom);

    // Stage 1: Active cycle
    manager.save_cycle_progress(
        WashProgram::NORMAL_WASH,
        WaterLevel::LOW_LEVEL,
        false,
        2,
        false
    );

    // Stage 2: Cycle finishes
    manager.save_cycle_finished(WashProgram::NORMAL_WASH, WaterLevel::LOW_LEVEL, false);

    CyclePersistenceManager reboot_manager(mock_eeprom);
    CycleSnapshot loaded{};
    bool found = reboot_manager.load_latest(loaded);

    EXPECT_TRUE(found);
    EXPECT_EQ(loaded.seq_num, 2);
    EXPECT_FALSE(loaded.is_running);
    EXPECT_EQ(loaded.step_index, 0);
}

TEST_F(CyclePersistenceManagerTest, DistributesWritesAcrossAll64Slots)
{
    CyclePersistenceManager manager(mock_eeprom);

    // Write 130 snapshots (more than two full rotations of 64 slots)
    for (uint16_t i = 0; i < 130; ++i) {
        manager.save_cycle_progress(
            WashProgram::NORMAL_WASH,
            WaterLevel::LOW_LEVEL,
            false,
            static_cast<uint8_t>(i % 5),
            false
        );
    }

    EXPECT_EQ(manager.get_current_seq_num(), 130);

    // Verify recovery after 130 writes
    CyclePersistenceManager reboot_manager(mock_eeprom);
    CycleSnapshot loaded{};
    bool found = reboot_manager.load_latest(loaded);

    EXPECT_TRUE(found);
    EXPECT_EQ(loaded.seq_num, 130);
    EXPECT_EQ(loaded.step_index, static_cast<uint8_t>(129 % 5));
    EXPECT_TRUE(loaded.is_running);
}

TEST_F(CyclePersistenceManagerTest, RejectsCorruptedChecksumAndFallsBackToPreviousValidSlot)
{
    CyclePersistenceManager manager(mock_eeprom);

    manager.save_cycle_progress(WashProgram::NORMAL_WASH, WaterLevel::LOW_LEVEL, false, 1, false);
    manager.save_cycle_progress(WashProgram::NORMAL_WASH, WaterLevel::LOW_LEVEL, false, 2, false);

    // Corrupt the newest slot (seq 2 at slot index 2)
    uint8_t corrupt_slot = manager.get_current_slot();
    uint16_t corrupt_addr = corrupt_slot * CyclePersistenceManager::SLOT_SIZE;
    uint8_t orig_byte = mock_eeprom.get_byte(corrupt_addr + 4); // Corrupt step_index or CRC
    mock_eeprom.set_byte(corrupt_addr + 4, orig_byte ^ 0xFF);

    // Reboot manager
    CyclePersistenceManager reboot_manager(mock_eeprom);
    CycleSnapshot loaded{};
    bool found = reboot_manager.load_latest(loaded);

    EXPECT_TRUE(found);
    // Should safely fallback to sequence 1!
    EXPECT_EQ(loaded.seq_num, 1);
    EXPECT_EQ(loaded.step_index, 1);
}

TEST_F(CyclePersistenceManagerTest, RejectsCorruptedMagicByte)
{
    CyclePersistenceManager manager(mock_eeprom);
    manager.save_user_selection(WashProgram::RINSE_ONLY, WaterLevel::MEDIUM_LEVEL, false);

    uint8_t slot = manager.get_current_slot();
    uint16_t addr = slot * CyclePersistenceManager::SLOT_SIZE;
    mock_eeprom.set_byte(addr + 8, 0x00); // Invalidate magic byte

    CyclePersistenceManager reboot_manager(mock_eeprom);
    CycleSnapshot loaded{};
    EXPECT_FALSE(reboot_manager.load_latest(loaded));
}

TEST_F(CyclePersistenceManagerTest, RejectsInvalidEnumValues)
{
    CyclePersistenceManager manager(mock_eeprom);
    manager.save_user_selection(WashProgram::RINSE_ONLY, WaterLevel::MEDIUM_LEVEL, false);

    uint8_t slot = manager.get_current_slot();
    uint16_t addr = slot * CyclePersistenceManager::SLOT_SIZE;

    // Set invalid program enum (99) and recompute CRC
    uint8_t buf[CyclePersistenceManager::SLOT_SIZE];
    mock_eeprom.read_block(addr, buf, CyclePersistenceManager::SLOT_SIZE);
    buf[2] = 99; // Invalid WashProgram
    buf[9] = CyclePersistenceManager::calculate_crc8(buf, 9);
    mock_eeprom.write_block(addr, buf, CyclePersistenceManager::SLOT_SIZE);

    CyclePersistenceManager reboot_manager(mock_eeprom);
    CycleSnapshot loaded{};
    EXPECT_FALSE(reboot_manager.load_latest(loaded));
}

TEST_F(CyclePersistenceManagerTest, HandlesSequenceNumberWrapAroundAt65535)
{
    CyclePersistenceManager manager(mock_eeprom);

    // Write a slot with seq 65535
    uint8_t buf[CyclePersistenceManager::SLOT_SIZE];
    buf[0] = 0xFF;
    buf[1] = 0xFF; // seq = 65535
    buf[2] = static_cast<uint8_t>(WashProgram::NORMAL_WASH);
    buf[3] = static_cast<uint8_t>(WaterLevel::HIGH_LEVEL);
    buf[4] = 1;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 1;
    buf[8] = CyclePersistenceManager::MAGIC_BYTE;
    buf[9] = CyclePersistenceManager::calculate_crc8(buf, 9);
    mock_eeprom.write_block(63 * CyclePersistenceManager::SLOT_SIZE, buf, CyclePersistenceManager::SLOT_SIZE);

    // Write a newer wrapped slot with seq 0 at slot 0
    buf[0] = 0x00;
    buf[1] = 0x00; // seq = 0 (after wrap)
    buf[2] = static_cast<uint8_t>(WashProgram::NORMAL_WASH);
    buf[3] = static_cast<uint8_t>(WaterLevel::HIGH_LEVEL);
    buf[4] = 2;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 1;
    buf[8] = CyclePersistenceManager::MAGIC_BYTE;
    buf[9] = CyclePersistenceManager::calculate_crc8(buf, 9);
    mock_eeprom.write_block(0, buf, CyclePersistenceManager::SLOT_SIZE);

    CyclePersistenceManager reboot_manager(mock_eeprom);
    CycleSnapshot loaded{};
    bool found = reboot_manager.load_latest(loaded);

    EXPECT_TRUE(found);
    EXPECT_EQ(loaded.seq_num, 0);
    EXPECT_EQ(loaded.step_index, 2);
}
