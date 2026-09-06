#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_eeprom_hal.hpp"
#include <cstring>

namespace mocks {

/**
 * @class MockEepromHAL
 * @brief GoogleMock and In-Memory Storage Fake for IEepromHAL.
 * Simulates the 1024-byte ATmega328P EEPROM in host test RAM with write endurance tracking.
 */
class MockEepromHAL : public hal::IEepromHAL {
private:
    static constexpr size_t EEPROM_SIZE = 1024;
    uint8_t storage_[EEPROM_SIZE] = {};
    uint32_t write_count_{0};

public:
    MockEepromHAL()
    {
        // Default factory state of erased EEPROM memory
        memset(storage_, 0xFF, EEPROM_SIZE);
        UseRealStorage();
    }

    MOCK_METHOD(uint8_t, read_byte, (uint16_t address), (override));
    MOCK_METHOD(void, write_byte, (uint16_t address, uint8_t value), (override));
    MOCK_METHOD(void, read_block, (uint16_t address, void* dest, size_t length), (override));
    MOCK_METHOD(void, write_block, (uint16_t address, const void* src, size_t length), (override));
    MOCK_METHOD(size_t, capacity, (), (const, override));

    void UseRealStorage()
    {
        ON_CALL(*this, capacity).WillByDefault(::testing::Return(EEPROM_SIZE));

        ON_CALL(*this, read_byte).WillByDefault([this](uint16_t address) {
            return (address < EEPROM_SIZE) ? storage_[address] : 0xFF;
        });

        ON_CALL(*this, write_byte).WillByDefault([this](uint16_t address, uint8_t value) {
            if (address < EEPROM_SIZE) {
                if (storage_[address] != value) {
                    storage_[address] = value;
                    write_count_++;
                }
            }
        });

        ON_CALL(*this, read_block).WillByDefault([this](uint16_t address, void* dest, size_t length) {
            if (dest != nullptr && address + length <= EEPROM_SIZE) {
                memcpy(dest, &storage_[address], length);
            }
        });

        ON_CALL(*this, write_block).WillByDefault([this](uint16_t address, const void* src, size_t length) {
            if (src != nullptr && address + length <= EEPROM_SIZE) {
                const uint8_t* p = static_cast<const uint8_t*>(src);
                for (size_t i = 0; i < length; ++i) {
                    if (storage_[address + i] != p[i]) {
                        storage_[address + i] = p[i];
                        write_count_++;
                    }
                }
            }
        });
    }

    // Test inspection and injection helpers:
    uint32_t get_write_count() const { return write_count_; }
    void reset_write_count() { write_count_ = 0; }
    uint8_t get_byte(uint16_t address) const { return (address < EEPROM_SIZE) ? storage_[address] : 0xFF; }
    void set_byte(uint16_t address, uint8_t value) { if (address < EEPROM_SIZE) storage_[address] = value; }
    const uint8_t* get_storage_buffer() const { return storage_; }
    void clear_all(uint8_t fill_val = 0xFF) { memset(storage_, fill_val, EEPROM_SIZE); }
};

} // namespace mocks
