#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_timer_hal.hpp"
#include "mocks/mock_digital_output.hpp"
#include "mocks/mock_water_level_sensor.hpp"
#include "controllers/fill_controller.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;

class FillControllerTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockTimerHAL> mock_timer;
    NiceMock<mocks::MockDigitalOutput> mock_valve_main;
    NiceMock<mocks::MockDigitalOutput> mock_valve_softener;
    NiceMock<mocks::MockWaterLevelSensor> mock_water_sensor;

    uint32_t simulated_time_ms{0};
    const uint32_t timeout_ms{10000}; // 10s for fast test

    controllers::FillController fill_ctrl{
        mock_timer,
        mock_valve_main,
        mock_valve_softener,
        mock_water_sensor,
        timeout_ms
    };

    void SetUp() override
    {
        simulated_time_ms = 0;
        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Invoke([this]() {
            return simulated_time_ms;
        }));
    }
};

TEST_F(FillControllerTest, StartsFillingMainWaterOnly)
{
    EXPECT_CALL(mock_valve_main, turn_on()).Times(1);
    EXPECT_CALL(mock_valve_softener, turn_off()).Times(1);

    fill_ctrl.start(domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_TRUE(fill_ctrl.is_active());
    EXPECT_FALSE(fill_ctrl.is_finished());
    EXPECT_FALSE(fill_ctrl.has_error());
}

TEST_F(FillControllerTest, StartsFillingWithSoftenerValveBothOpen)
{
    EXPECT_CALL(mock_valve_main, turn_on()).Times(1);
    EXPECT_CALL(mock_valve_softener, turn_on()).Times(1);

    fill_ctrl.start(domain::WaterLevel::MEDIUM_LEVEL, true);
    EXPECT_TRUE(fill_ctrl.is_active());
}

TEST_F(FillControllerTest, FinishesFillingWhenTargetLevelReached)
{
    fill_ctrl.start(domain::WaterLevel::LOW_LEVEL, false);

    ON_CALL(mock_water_sensor, is_level_reached(domain::WaterLevel::LOW_LEVEL)).WillByDefault(Return(false));
    fill_ctrl.update();
    EXPECT_TRUE(fill_ctrl.is_active());
    EXPECT_FALSE(fill_ctrl.is_finished());

    // Water reaches target level
    ON_CALL(mock_water_sensor, is_level_reached(domain::WaterLevel::LOW_LEVEL)).WillByDefault(Return(true));
    EXPECT_CALL(mock_valve_main, turn_off()).Times(1);
    EXPECT_CALL(mock_valve_softener, turn_off()).Times(1);

    fill_ctrl.update();
    EXPECT_FALSE(fill_ctrl.is_active());
    EXPECT_TRUE(fill_ctrl.is_finished());
    EXPECT_FALSE(fill_ctrl.has_error());
}

TEST_F(FillControllerTest, TriggersErrorOnTimeout)
{
    fill_ctrl.start(domain::WaterLevel::LOW_LEVEL, false);

    simulated_time_ms = 10500; // Past timeout
    EXPECT_CALL(mock_valve_main, turn_off()).Times(1);

    fill_ctrl.update();
    EXPECT_FALSE(fill_ctrl.is_active());
    EXPECT_FALSE(fill_ctrl.is_finished());
    EXPECT_TRUE(fill_ctrl.has_error());
}

TEST_F(FillControllerTest, StopImmediatelyTurnsOffValves)
{
    fill_ctrl.start(domain::WaterLevel::LOW_LEVEL, true);

    EXPECT_CALL(mock_valve_main, turn_off()).Times(1);
    EXPECT_CALL(mock_valve_softener, turn_off()).Times(1);

    fill_ctrl.stop();
    EXPECT_FALSE(fill_ctrl.is_active());
}
