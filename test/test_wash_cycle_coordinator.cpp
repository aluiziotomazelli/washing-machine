#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_timer_hal.hpp"
#include "mocks/mock_digital_output.hpp"
#include "mocks/mock_reversible_motor.hpp"
#include "mocks/mock_water_level_sensor.hpp"
#include "controllers/fill_controller.hpp"
#include "controllers/agitator.hpp"
#include "controllers/drain_controller.hpp"
#include "controllers/spin_controller.hpp"
#include "fsm/wash_cycle_coordinator.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::AtLeast;

class WashCycleCoordinatorTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockTimerHAL> mock_timer;
    NiceMock<mocks::MockDigitalOutput> mock_valve_main;
    NiceMock<mocks::MockDigitalOutput> mock_valve_softener;
    NiceMock<mocks::MockDigitalOutput> mock_drain_pump;
    NiceMock<mocks::MockReversibleMotor> mock_motor;
    NiceMock<mocks::MockWaterLevelSensor> mock_water_sensor;

    uint32_t simulated_time_ms{0};

    // Fast timings for unit tests
    fsm::CoordinatorConfig config{
        100, // 100ms stage settle pause
        2,   // 2s normal wash agitate
        1,   // 1s heavy agitate 1
        1,   // 1s heavy soak
        1,   // 1s heavy agitate 2
        1,   // 1s single rinse agitate
        1,   // 1s double rinse 1 agitate
        1,   // 1s double rinse interm spin
        1,   // 1s double rinse 2 agitate
        1,   // 1s double rinse 2 soak
        1,   // 1s double rinse 2 post agitate
        2    // 2s final spin
    };

    controllers::FillController fill_ctrl{mock_timer, mock_valve_main, mock_valve_softener, mock_water_sensor, 5000};
    controllers::Agitator agitator{mock_timer, mock_motor};
    controllers::DrainController drain_ctrl{mock_timer, mock_drain_pump, mock_water_sensor, 5000};
    controllers::SpinConfig spin_config{100, 100, 100, 100, 200};
    controllers::SpinController spin_ctrl{mock_timer, mock_drain_pump, mock_motor, spin_config};

    fsm::WashCycleCoordinator coordinator{
        mock_timer,
        fill_ctrl,
        agitator,
        drain_ctrl,
        spin_ctrl,
        config
    };

    void SetUp() override
    {
        simulated_time_ms = 0;
        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Invoke([this]() {
            return simulated_time_ms;
        }));
    }
};

TEST_F(WashCycleCoordinatorTest, InitializesInIdleState)
{
    coordinator.init();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::IDLE);
    EXPECT_EQ(coordinator.get_current_stage(), domain::WashStage::IDLE);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::NONE);
}

TEST_F(WashCycleCoordinatorTest, StartsSpinOnlyRecipeAndSequencesDrainThenSpin)
{
    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::RUNNING);
    EXPECT_EQ(coordinator.get_current_stage(), domain::WashStage::SPIN);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::DRAIN);
    EXPECT_TRUE(drain_ctrl.is_active());

    // Tub becomes empty -> enters bleeding phase
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(true));
    coordinator.update();

    // Bleed duration expires (30s default) -> advances to SPIN_FINAL
    simulated_time_ms += 31000;
    coordinator.update();

    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_FINAL);
    EXPECT_TRUE(spin_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, StartsSingleRinseRecipeWithoutSoftener)
{
    coordinator.start_cycle(domain::WashProgram::RINSE_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::RUNNING);
    EXPECT_EQ(coordinator.get_current_stage(), domain::WashStage::RINSE);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::FILL_MAIN);
    EXPECT_TRUE(fill_ctrl.is_active());

    // Level reached -> settle pause
    ON_CALL(mock_water_sensor, is_level_reached(domain::WaterLevel::LOW_LEVEL)).WillByDefault(Return(true));
    coordinator.update();
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SETTLE_PAUSE);

    // Settle pause expires -> starts AGITATE_NORMAL
    simulated_time_ms += 150;
    coordinator.update();
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::AGITATE_NORMAL);
    EXPECT_TRUE(agitator.is_active());
}

TEST_F(WashCycleCoordinatorTest, PausesAndResumesDelegatingToActiveController)
{
    coordinator.start_cycle(domain::WashProgram::RINSE_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_TRUE(fill_ctrl.is_active());

    // Pause cycle
    coordinator.pause_cycle();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::PAUSED);
    EXPECT_TRUE(fill_ctrl.is_paused());

    // Resume cycle
    coordinator.resume_cycle();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::RUNNING);
    EXPECT_FALSE(fill_ctrl.is_paused());
}

TEST_F(WashCycleCoordinatorTest, AdvanceStepSkipsCurrentProcessToNextStep)
{
    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::DRAIN);

    // Long press / button skip
    coordinator.advance_step();
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_FINAL);
    EXPECT_TRUE(spin_ctrl.is_active());

    // Advance again -> cycle finished
    coordinator.advance_step();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::FINISHED);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::FINISHED);
}

TEST_F(WashCycleCoordinatorTest, StopCycleHaltsAllControllersAndReturnsToIdle)
{
    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_TRUE(drain_ctrl.is_active());

    coordinator.stop_cycle();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::IDLE);
    EXPECT_EQ(coordinator.get_current_stage(), domain::WashStage::IDLE);
    EXPECT_FALSE(drain_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, ErrorInControllerTransitionsCoordinatorToErrorState)
{
    coordinator.start_cycle(domain::WashProgram::RINSE_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_TRUE(fill_ctrl.is_active());

    // Force fill controller timeout error
    simulated_time_ms = 6000;
    coordinator.update();

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::ERROR);
    EXPECT_EQ(coordinator.get_error(), domain::MachineError::FILL_TIMEOUT);
    EXPECT_FALSE(fill_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, DrainTimeoutTransitionsToDrainTimeoutError)
{
    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_TRUE(drain_ctrl.is_active());

    // Force drain controller timeout (drain timeout is 5000ms in test mock)
    simulated_time_ms = 6000;
    coordinator.update();

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::ERROR);
    EXPECT_EQ(coordinator.get_error(), domain::MachineError::DRAIN_TIMEOUT);
    EXPECT_FALSE(drain_ctrl.is_active());
}
