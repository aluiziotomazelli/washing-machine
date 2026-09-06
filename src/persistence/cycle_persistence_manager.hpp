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

    explicit CyclePersistenceManager(
        hal::IEepromHAL& eeprom_hal,
        uint16_t start_address = DEFAULT_START_ADDRESS,
        uint8_t slot_count = DEFAULT_SLOT_COUNT
    );

    bool load_latest(CycleSnapshot& out_snapshot) override;
    void save_user_selection(WashProgram program, WaterLevel level, bool softener_enabled) override;
    void save_cycle_progress(
        WashProgram program,
        WaterLevel level,
        bool softener_enabled,
        uint8_t step_index,
        bool in_rinse_subcycle,
        bool is_paused = false
    ) override;
    void save_cycle_finished(WashProgram program, WaterLevel level, bool softener_enabled) override;

    uint16_t get_current_seq_num() const { return current_seq_num_; }
    uint8_t get_current_slot() const { return current_slot_; }
    bool has_valid_snapshot() const { return has_valid_snapshot_; }
    const CycleSnapshot& get_cached_snapshot() const { return cached_snapshot_; }

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
