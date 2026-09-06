#pragma once

#include <gmock/gmock.h>
#include "persistence/interfaces/i_cycle_persistence.hpp"

namespace mocks {

/**
 * @class MockCyclePersistence
 * @brief GoogleMock implementation of ICyclePersistence for testing coordinator and panel persistence calls.
 */
class MockCyclePersistence : public persistence::ICyclePersistence {
public:
    MOCK_METHOD(bool, load_latest, (persistence::CycleSnapshot& out_snapshot), (override));
    MOCK_METHOD(void, save_user_selection, (domain::WashProgram program, domain::WaterLevel level, bool softener_enabled), (override));
    MOCK_METHOD(void, save_cycle_progress, (domain::WashProgram program, domain::WaterLevel level, bool softener_enabled, uint8_t step_index, bool in_rinse_subcycle, bool is_paused), (override));
    MOCK_METHOD(void, save_cycle_finished, (domain::WashProgram program, domain::WaterLevel level, bool softener_enabled), (override));
};

} // namespace mocks
