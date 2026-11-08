#pragma once

#include <stdint.h>
#include <stddef.h>
#include "interfaces/i_cycle_persistence.hpp"
#include "../hal/interfaces/i_eeprom_hal.hpp"

namespace persistence {

/**
 * @class CyclePersistenceManager
 * @brief High-reliability wear-leveling persistence manager using a circular ring buffer.
 *
 * Distributes EEPROM writes evenly across 64 circular slots without any fixed-address pointer.
 * On boot, a full scan locates the latest valid snapshot using CRC-8 verification and
 * monotonic sequence numbering.
 */
class CyclePersistenceManager : public ICyclePersistence {
public:
    static constexpr uint8_t DEFAULT_SLOT_COUNT = 64;
    static constexpr size_t SLOT_SIZE = 10;
    static constexpr uint16_t DEFAULT_START_ADDRESS = 0;
    static constexpr uint8_t MAGIC_BYTE = 0xA5;

    /**
     * @brief Construct a persistence manager over an EEPROM HAL.
     * @param eeprom_hal EEPROM abstraction used for storage.
     * @param start_address Byte offset of the first ring slot.
     * @param slot_count Number of circular slots (default 64).
     */
    explicit CyclePersistenceManager(
        hal::IEepromHAL& eeprom_hal,
        uint16_t start_address = DEFAULT_START_ADDRESS,
        uint8_t slot_count = DEFAULT_SLOT_COUNT
    );

    /** @copydoc ICyclePersistence::load_latest */
    bool load_latest(CycleSnapshot& out_snapshot) override;
    /** @copydoc ICyclePersistence::save_user_selection */
    void save_user_selection(WashProgram program, WaterLevel level, bool softener_enabled) override;
    /** @copydoc ICyclePersistence::save_cycle_progress */
    void save_cycle_progress(
        WashProgram program,
        WaterLevel level,
        bool softener_enabled,
        uint8_t step_index,
        bool in_rinse_subcycle,
        bool is_paused = false
    ) override;
    /** @copydoc ICyclePersistence::save_cycle_finished */
    void save_cycle_finished(WashProgram program, WaterLevel level, bool softener_enabled) override;

    /**
     * @brief Get the monotonic sequence number of the last written snapshot.
     * @return Sequence number.
     */
    uint16_t get_current_seq_num() const { return current_seq_num_; }

    /**
     * @brief Get the ring-buffer slot index of the last written snapshot.
     * @return Slot index.
     */
    uint8_t get_current_slot() const { return current_slot_; }

    /**
     * @brief Check whether a valid snapshot was found during the last boot scan.
     * @return true when recovery data is available.
     */
    bool has_valid_snapshot() const { return has_valid_snapshot_; }

    /**
     * @brief Get the cached snapshot of the most recent valid record.
     * @return Constant reference to the cached CycleSnapshot.
     */
    const CycleSnapshot& get_cached_snapshot() const { return cached_snapshot_; }

    /**
     * @brief Compute the CRC-8 used to validate persisted snapshots.
     * @param data Pointer to the byte buffer.
     * @param length Number of bytes to process.
     * @return CRC-8 checksum.
     */
    static uint8_t calculate_crc8(const uint8_t* data, size_t length);

private:
    void write_snapshot(const CycleSnapshot& snapshot);
    bool deserialize_slot(uint16_t address, CycleSnapshot& out_snapshot);

    hal::IEepromHAL& eeprom_hal_;
    uint16_t start_address_;
    uint8_t slot_count_;
    uint16_t current_seq_num_{0};
    uint8_t current_slot_{0};
    bool has_valid_snapshot_{false};
    CycleSnapshot cached_snapshot_{};
};

} // namespace persistence
