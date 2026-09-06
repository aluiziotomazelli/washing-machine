#include "cycle_persistence_manager.hpp"

namespace persistence {

CyclePersistenceManager::CyclePersistenceManager(
    hal::IEepromHAL& eeprom_hal,
    uint16_t start_address,
    uint8_t slot_count)
    : eeprom_hal_(eeprom_hal)
    , start_address_(start_address)
    , slot_count_(slot_count > 0 ? slot_count : DEFAULT_SLOT_COUNT)
{
}

uint8_t CyclePersistenceManager::calculate_crc8(const uint8_t* data, size_t length)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool CyclePersistenceManager::deserialize_slot(uint16_t address, CycleSnapshot& out_snapshot)
{
    uint8_t buf[SLOT_SIZE];
    eeprom_hal_.read_block(address, buf, SLOT_SIZE);

    // Verify magic byte marker
    if (buf[8] != MAGIC_BYTE) {
        return false;
    }

    // Verify CRC-8 checksum calculated over payload bytes 0..8
    uint8_t computed_crc = calculate_crc8(buf, 9);
    if (computed_crc != buf[9]) {
        return false;
    }

    // Validate enum and boolean bounds
    if (buf[2] >= static_cast<uint8_t>(WashProgram::COUNT)) {
        return false;
    }
    if (buf[3] >= static_cast<uint8_t>(WaterLevel::COUNT)) {
        return false;
    }
    // buf[5] in_rinse (0-1), buf[6] softener (0-1), buf[7] run_state (0=STOPPED, 1=RUNNING, 2=PAUSED)
    if (buf[5] > 1 || buf[6] > 1 || buf[7] > 2) {
        return false;
    }

    out_snapshot.seq_num = static_cast<uint16_t>(buf[0] | (static_cast<uint16_t>(buf[1]) << 8));
    out_snapshot.program = static_cast<WashProgram>(buf[2]);
    out_snapshot.level = static_cast<WaterLevel>(buf[3]);
    out_snapshot.step_index = buf[4];
    out_snapshot.in_rinse_subcycle = (buf[5] != 0);
    out_snapshot.softener_enabled = (buf[6] != 0);
    out_snapshot.is_running = (buf[7] == 1 || buf[7] == 2);
    out_snapshot.is_paused = (buf[7] == 2);
    out_snapshot.checksum = buf[9];

    return true;
}

bool CyclePersistenceManager::load_latest(CycleSnapshot& out_snapshot)
{
    bool found = false;
    uint16_t best_seq = 0;
    uint8_t best_slot = 0;
    CycleSnapshot best_snapshot{};

    for (uint8_t i = 0; i < slot_count_; ++i) {
        uint16_t addr = start_address_ + (i * SLOT_SIZE);
        CycleSnapshot candidate{};
        if (deserialize_slot(addr, candidate)) {
            if (!found) {
                found = true;
                best_seq = candidate.seq_num;
                best_slot = i;
                best_snapshot = candidate;
            } else {
                int16_t diff = static_cast<int16_t>(candidate.seq_num - best_seq);
                if (diff > 0) {
                    best_seq = candidate.seq_num;
                    best_slot = i;
                    best_snapshot = candidate;
                }
            }
        }
    }

    if (found) {
        current_seq_num_ = best_seq;
        current_slot_ = best_slot;
        has_valid_snapshot_ = true;
        cached_snapshot_ = best_snapshot;
        out_snapshot = best_snapshot;
        return true;
    }

    current_seq_num_ = 0;
    current_slot_ = 0;
    has_valid_snapshot_ = false;
    return false;
}

void CyclePersistenceManager::write_snapshot(const CycleSnapshot& snapshot)
{
    current_seq_num_++;
    current_slot_ = current_seq_num_ % slot_count_;
    uint16_t addr = start_address_ + (current_slot_ * SLOT_SIZE);

    uint8_t run_state = 0;
    if (snapshot.is_running) {
        run_state = snapshot.is_paused ? 2 : 1;
    }

    uint8_t buf[SLOT_SIZE];
    buf[0] = static_cast<uint8_t>(current_seq_num_ & 0xFF);
    buf[1] = static_cast<uint8_t>((current_seq_num_ >> 8) & 0xFF);
    buf[2] = static_cast<uint8_t>(snapshot.program);
    buf[3] = static_cast<uint8_t>(snapshot.level);
    buf[4] = snapshot.step_index;
    buf[5] = snapshot.in_rinse_subcycle ? 1 : 0;
    buf[6] = snapshot.softener_enabled ? 1 : 0;
    buf[7] = run_state;
    buf[8] = MAGIC_BYTE;
    buf[9] = calculate_crc8(buf, 9);

    eeprom_hal_.write_block(addr, buf, SLOT_SIZE);

    cached_snapshot_ = snapshot;
    cached_snapshot_.seq_num = current_seq_num_;
    cached_snapshot_.checksum = buf[9];
    has_valid_snapshot_ = true;
}

void CyclePersistenceManager::save_user_selection(WashProgram program, WaterLevel level, bool softener_enabled)
{
    CycleSnapshot s{};
    s.program = program;
    s.level = level;
    s.softener_enabled = softener_enabled;
    s.step_index = 0;
    s.in_rinse_subcycle = false;
    s.is_running = false;
    s.is_paused = false;
    write_snapshot(s);
}

void CyclePersistenceManager::save_cycle_progress(
    WashProgram program,
    WaterLevel level,
    bool softener_enabled,
    uint8_t step_index,
    bool in_rinse_subcycle,
    bool is_paused)
{
    CycleSnapshot s{};
    s.program = program;
    s.level = level;
    s.softener_enabled = softener_enabled;
    s.step_index = step_index;
    s.in_rinse_subcycle = in_rinse_subcycle;
    s.is_running = true;
    s.is_paused = is_paused;
    write_snapshot(s);
}

void CyclePersistenceManager::save_cycle_finished(WashProgram program, WaterLevel level, bool softener_enabled)
{
    CycleSnapshot s{};
    s.program = program;
    s.level = level;
    s.softener_enabled = softener_enabled;
    s.step_index = 0;
    s.in_rinse_subcycle = false;
    s.is_running = false;
    write_snapshot(s);
}

} // namespace persistence
