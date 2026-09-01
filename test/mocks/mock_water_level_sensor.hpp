#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_water_level_sensor.hpp"

namespace mocks {

class MockWaterLevelSensor : public hal::IWaterLevelSensor {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, is_level_reached, (domain::WaterLevel target), (const, override));
    MOCK_METHOD(domain::WaterLevel, get_current_level, (), (const, override));
    MOCK_METHOD(bool, is_empty, (), (const, override));
};

} // namespace mocks
