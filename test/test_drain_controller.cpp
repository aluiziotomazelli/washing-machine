#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_timer_hal.hpp"
#include "mocks/mock_digital_output.hpp"
#include "mocks/mock_water_level_sensor.hpp"
#include "controllers/drain_controller.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;

class DrainControllerTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockTimerHAL> mock_timer;
    NiceMock<mocks::MockDigitalOutput> mock_drain_pump;
    NiceMock<mocks::MockWaterLevelSensor> mock_water_sensor;

    uint32_t simulated_time_ms{0};
    const uint32_t timeout_ms{10000}; // 10s for test

    controllers::DrainController drain_ctrl{
        mock_timer,
        mock_drain_pump,
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

TEST_F(DrainControllerTest, StartsDrainingAndTurnsPumpOn)
{
    EXPECT_CALL(mock_drain_pump, turn_on()).Times(1);

    drain_ctrl.start(3000); // 3s bleed
    EXPECT_TRUE(drain_ctrl.is_active());
    EXPECT_FALSE(drain_ctrl.is_bleeding());
    EXPECT_FALSE(drain_ctrl.is_finished());
    EXPECT_FALSE(drain_ctrl.has_error());
}

TEST_F(DrainControllerTest, EntersBleedingPhaseWhenSensorDetectsEmptyTub)
{
    drain_ctrl.start(3000);

    // Tub is not empty yet
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(false));
    drain_ctrl.update();
    EXPECT_FALSE(drain_ctrl.is_bleeding());

    // Tub becomes empty at 2000ms
    simulated_time_ms = 2000;
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(true));
    drain_ctrl.update();
    EXPECT_TRUE(drain_ctrl.is_bleeding());
    EXPECT_FALSE(drain_ctrl.is_finished());

    // During bleed (at 4000ms, only 2000ms of 3000ms elapsed)
    simulated_time_ms = 4000;
    drain_ctrl.update();
    EXPECT_FALSE(drain_ctrl.is_finished());

    // Bleed finishes at 5000ms (3000ms after empty detected)
    simulated_time_ms = 5000;
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    drain_ctrl.update();
    EXPECT_TRUE(drain_ctrl.is_finished());
    EXPECT_FALSE(drain_ctrl.is_active());
}

TEST_F(DrainControllerTest, TriggersErrorIfTubNeverEmptiesBeforeTimeout)
{
    drain_ctrl.start(3000);

    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(false));

    simulated_time_ms = 10100; // Past 10s timeout
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);

    drain_ctrl.update();
    EXPECT_TRUE(drain_ctrl.has_error());
    EXPECT_FALSE(drain_ctrl.is_active());
}

TEST_F(DrainControllerTest, StopImmediatelyTurnsOffPump)
{
    drain_ctrl.start(3000);

    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    drain_ctrl.stop();
    EXPECT_FALSE(drain_ctrl.is_active());
}

TEST_F(DrainControllerTest, PausesAndResumesDuringDrainingAndBleeding)
{
    drain_ctrl.start(3000); // 3s bleed

    // Draining for 2 seconds (tub still has water)
    simulated_time_ms = 2000;
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(false));
    drain_ctrl.update();

    // Pause draining
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    drain_ctrl.pause();
    EXPECT_TRUE(drain_ctrl.is_paused());

    // While paused for 15 seconds, timeout should NOT trigger
    simulated_time_ms = 17000;
    drain_ctrl.update();
    EXPECT_FALSE(drain_ctrl.has_error());

    // Resume draining: pump restarts
    EXPECT_CALL(mock_drain_pump, turn_on()).Times(1);
    drain_ctrl.resume();
    EXPECT_FALSE(drain_ctrl.is_paused());

    // Tub becomes empty at 19000ms
    simulated_time_ms = 19000;
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(true));
    drain_ctrl.update();
    EXPECT_TRUE(drain_ctrl.is_bleeding());

    // 1s into bleed (19000 + 1000 = 20000), pause again during bleed!
    simulated_time_ms = 20000;
    drain_ctrl.update();
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    drain_ctrl.pause();
    EXPECT_TRUE(drain_ctrl.is_paused());

    // Resume bleed: needs 2s more to finish 3s bleed
    EXPECT_CALL(mock_drain_pump, turn_on()).Times(1);
    simulated_time_ms = 25000;
    drain_ctrl.resume();

    simulated_time_ms = 26000; // 1s more (2s total bleed)
    drain_ctrl.update();
    EXPECT_FALSE(drain_ctrl.is_finished());

    simulated_time_ms = 27000; // 2s more (3s total bleed) -> finishes!
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    drain_ctrl.update();
    EXPECT_TRUE(drain_ctrl.is_finished());
}
