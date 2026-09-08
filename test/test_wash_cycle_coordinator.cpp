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
#include "mocks/mock_vibration_monitor.hpp"
#include "mocks/mock_cycle_persistence.hpp"
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
    NiceMock<mocks::MockVibrationMonitor> mock_vib;

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
        2,   // 2s final spin
        1,   // 1 max unbalance recovery
        1,   // 1s unbalance agitate
        domain::WaterLevel::LOW_LEVEL
    };

    controllers::FillController fill_ctrl{mock_timer, mock_valve_main, mock_valve_softener, mock_water_sensor, 5000};
    controllers::Agitator agitator{mock_timer, mock_motor};
    controllers::DrainController drain_ctrl{mock_timer, mock_drain_pump, mock_water_sensor, 5000};
    controllers::SpinConfig spin_config{100, 100, 100, 200, nullptr, 0, 2};
    controllers::SpinController spin_ctrl{mock_timer, mock_drain_pump, mock_motor, spin_config, &mock_vib};

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

    // Bleed duration expires (20s default) -> advances to SPIN_FINAL
    simulated_time_ms += 21000;
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

TEST_F(WashCycleCoordinatorTest, SeamlessHandoverFromDrainToSpinNeverTurnsOffPump)
{
    // Start with tub containing water so pump turns on
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(false));
    EXPECT_CALL(mock_drain_pump, turn_on()).Times(AtLeast(1));

    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::DRAIN);

    // Tub becomes empty at 2000ms
    simulated_time_ms = 2000;
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(true));
    coordinator.update();

    // During the entire transition from DRAIN to SPIN, turn_off() MUST NOT be called!
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(0);

    // Bleed duration expires (30s) -> transitions into SPIN_FINAL
    simulated_time_ms += 31000;
    coordinator.update();

    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_FINAL);
    EXPECT_TRUE(spin_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, HandoverFromDrainToSpinAndSpinToFill)
{
    // Normal wash: Step 0 is FILL, Step 1 is AGITATE, Step 2 is DRAIN, Step 3 is SPIN_INTERMEDIATE
    coordinator.start_cycle(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::LOW_LEVEL, false);

    // Skip FILL and AGITATE to reach DRAIN (Step 2)
    coordinator.advance_step(); // moves to Step 1 (AGITATE)
    coordinator.advance_step(); // moves to Step 2 (DRAIN)
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::DRAIN);

    // Tub becomes empty and bleed completes -> transitions into SPIN_INTERMEDIATE (smooth handover)
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(true));
    coordinator.update();

    simulated_time_ms += 31000;
    coordinator.update();

    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_INTERMEDIATE);
    EXPECT_TRUE(spin_ctrl.is_active());

    // Clutch engage (100ms) finishes -> active spinning
    simulated_time_ms += 100;
    coordinator.update();

    // Advancing during active spin enters STOP_COASTING with pump on
    coordinator.advance_step();
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_INTERMEDIATE);
    EXPECT_TRUE(spin_ctrl.is_active());

    // 100ms into 200ms coast-down: still coasting, fill has NOT started
    simulated_time_ms += 100;
    coordinator.update();
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_INTERMEDIATE);
    EXPECT_FALSE(fill_ctrl.is_active());

    // Coast-down completes (200ms total): pump is turned off, clutch released, advances to RINSE / FILL_MAIN
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(AtLeast(1));
    simulated_time_ms += 100;
    coordinator.update();

    EXPECT_EQ(coordinator.get_current_stage(), domain::WashStage::RINSE);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::FILL_MAIN);
    EXPECT_TRUE(fill_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, PauseDuringSpinCoastsDownAndTurnsOffPumpWhilePaused)
{
    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    coordinator.advance_step(); // moves to SPIN_FINAL
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_FINAL);

    // Clutch engages (100ms) -> spinning
    simulated_time_ms += 100;
    coordinator.update();
    EXPECT_TRUE(spin_ctrl.is_active());

    // Pause while spinning: enters PAUSE_COASTING with pump held on
    coordinator.pause_cycle();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::PAUSED);
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::PAUSE_COASTING);

    // 100ms into coast-down while paused: pump still on
    simulated_time_ms += 100;
    coordinator.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::PAUSE_COASTING);

    // Coast-down finishes (200ms total): pump is turned off while paused!
    EXPECT_CALL(mock_drain_pump, turn_off()).Times(1);
    simulated_time_ms += 100;
    coordinator.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::PAUSED);
    EXPECT_TRUE(spin_ctrl.is_paused());

    // Resume re-engages clutch and pump safely
    EXPECT_CALL(mock_drain_pump, turn_on()).Times(1);
    coordinator.resume_cycle();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::RUNNING);
    EXPECT_FALSE(spin_ctrl.is_paused());
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::CLUTCH_ENGAGE);
}

TEST_F(WashCycleCoordinatorTest, AdvanceStepWhilePausedDuringSpinSkipsImmediately)
{
    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    coordinator.advance_step(); // moves to SPIN_FINAL

    // Clutch engages (100ms) -> spinning
    simulated_time_ms += 100;
    coordinator.update();

    // Pause and wait for full standstill
    coordinator.pause_cycle();
    simulated_time_ms += 200;
    coordinator.update();
    EXPECT_EQ(spin_ctrl.get_sub_phase(), controllers::SpinSubPhase::PAUSED);

    // Advancing while already at 0 RPM stationary in pause skips immediately without waiting
    coordinator.advance_step();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::FINISHED);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::FINISHED);
}

TEST_F(WashCycleCoordinatorTest, ResumesCycleAfterFillTimeoutError)
{
    coordinator.start_cycle(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::FILL_MAIN);
    EXPECT_TRUE(fill_ctrl.is_active());

    // Advance beyond 5000ms fill timeout
    simulated_time_ms += 6000;
    coordinator.update();

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::ERROR);
    EXPECT_EQ(coordinator.get_error(), domain::MachineError::FILL_TIMEOUT);
    EXPECT_FALSE(fill_ctrl.is_active());
    EXPECT_TRUE(fill_ctrl.has_error());

    // User opens tap and resumes cycle
    coordinator.resume_cycle();

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::RUNNING);
    EXPECT_EQ(coordinator.get_error(), domain::MachineError::NONE);
    EXPECT_FALSE(fill_ctrl.has_error());
    EXPECT_TRUE(fill_ctrl.is_active());
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::FILL_MAIN);
}

TEST_F(WashCycleCoordinatorTest, TransitionsToUnbalancedLoadErrorAndResumes)
{
    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);

    // Skip DRAIN to reach SPIN_FINAL
    coordinator.advance_step();
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_FINAL);
    EXPECT_TRUE(spin_ctrl.is_active());

    // Clutch engage (100ms)
    simulated_time_ms += 100;
    coordinator.update();

    // Trigger persistent unbalance to exhaust SpinController's 2 dry retries
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(true));

    // Trip 1 -> RETRY_COASTING (200ms) -> Clutch (100ms)
    simulated_time_ms += 50;  coordinator.update();
    simulated_time_ms += 200; coordinator.update();
    simulated_time_ms += 100; coordinator.update();

    // Trip 2 -> RETRY_COASTING (200ms) -> Clutch (100ms)
    simulated_time_ms += 50;  coordinator.update();
    simulated_time_ms += 200; coordinator.update();
    simulated_time_ms += 100; coordinator.update();

    // Trip 3 (exhausts 2 dry retries) -> enters STOP_COASTING (200ms) -> completes and transitions to RECOVERY_FILL
    simulated_time_ms += 50;  coordinator.update();
    simulated_time_ms += 200; coordinator.update();

    // Coordinator intercepts spin error and starts hydraulic recovery!
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::RECOVERY_FILL);
    EXPECT_EQ(coordinator.get_current_stage(), domain::WashStage::SPIN); // Preserves SPIN stage indicator!
    EXPECT_TRUE(fill_ctrl.is_active());
    EXPECT_EQ(coordinator.get_unbalance_recoveries(), 1);

    // Water level reaches LOW_LEVEL -> Fill finishes -> Transitions to RECOVERY_AGITATE
    ON_CALL(mock_water_sensor, is_level_reached(domain::WaterLevel::LOW_LEVEL)).WillByDefault(Return(true));
    coordinator.update();
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::RECOVERY_AGITATE);
    EXPECT_TRUE(agitator.is_active());

    // Agitation finishes (1s configured) -> Transitions to RECOVERY_DRAIN
    simulated_time_ms += 1000;
    coordinator.update();
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::RECOVERY_DRAIN);
    EXPECT_TRUE(drain_ctrl.is_active());

    // Water drained and bleed finishes -> Transitions back to SPIN_FINAL with fresh retries!
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(true));
    coordinator.update();
    simulated_time_ms += 30000; // bleed time
    coordinator.update();

    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_FINAL);
    EXPECT_TRUE(spin_ctrl.is_active());
    EXPECT_FALSE(spin_ctrl.has_error());

    // Laundry is now redistributed! Normal spin is active without errors
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    coordinator.advance_step();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::FINISHED);
}

TEST_F(WashCycleCoordinatorTest, HydraulicRecoveryExhaustionLatchesUnbalancedLoadError)
{
    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    coordinator.advance_step(); // moves to SPIN_FINAL

    // Fast-forward unbalance to exhaust dry retries
    simulated_time_ms += 100; coordinator.update();
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(true));

    // Exhaust 2 dry retries
    simulated_time_ms += 50;  coordinator.update();
    simulated_time_ms += 200; coordinator.update();
    simulated_time_ms += 100; coordinator.update();
    simulated_time_ms += 50;  coordinator.update();
    simulated_time_ms += 200; coordinator.update();
    simulated_time_ms += 100; coordinator.update();
    simulated_time_ms += 50;  coordinator.update();
    simulated_time_ms += 200; coordinator.update(); // triggers RECOVERY_FILL

    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::RECOVERY_FILL);

    // Fast-forward recovery steps (Fill -> Agitate -> Drain)
    ON_CALL(mock_water_sensor, is_level_reached(domain::WaterLevel::LOW_LEVEL)).WillByDefault(Return(true));
    coordinator.update(); // moves to RECOVERY_AGITATE
    simulated_time_ms += 1000;
    coordinator.update(); // moves to RECOVERY_DRAIN
    ON_CALL(mock_water_sensor, is_empty()).WillByDefault(Return(true));
    coordinator.update();
    simulated_time_ms += 30000;
    coordinator.update(); // resumes SPIN_FINAL

    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_FINAL);

    // In the second spin attempt, unbalance persists!
    simulated_time_ms += 100; coordinator.update();
    simulated_time_ms += 50;  coordinator.update();
    simulated_time_ms += 200; coordinator.update();
    simulated_time_ms += 100; coordinator.update();
    simulated_time_ms += 50;  coordinator.update();
    simulated_time_ms += 200; coordinator.update();
    simulated_time_ms += 100; coordinator.update();
    simulated_time_ms += 50;  coordinator.update();
    simulated_time_ms += 200; coordinator.update(); // dry retries exhausted again -> complete coast down

    // Since hydraulic recovery was already attempted once, it now latches ERROR!
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::ERROR);
    EXPECT_EQ(coordinator.get_error(), domain::MachineError::UNBALANCED_LOAD);

    // User arranges laundry manually and presses Start/Pause to resume
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(false));
    coordinator.resume_cycle();

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::RUNNING);
    EXPECT_EQ(coordinator.get_error(), domain::MachineError::NONE);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::SPIN_FINAL);
    EXPECT_TRUE(spin_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, NoRecoveryWhenMaxRecoveriesIsZero)
{
    fsm::CoordinatorConfig zero_rec_cfg = config;
    zero_rec_cfg.max_unbalance_recoveries = 0;

    fsm::WashCycleCoordinator direct_coord{
        mock_timer, fill_ctrl, agitator, drain_ctrl, spin_ctrl, zero_rec_cfg
    };

    direct_coord.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);
    direct_coord.advance_step(); // moves to SPIN_FINAL

    // Fast-forward unbalance to exhaust dry retries
    simulated_time_ms += 100; direct_coord.update();
    ON_CALL(mock_vib, is_critical_unbalance()).WillByDefault(Return(true));

    simulated_time_ms += 50;  direct_coord.update();
    simulated_time_ms += 200; direct_coord.update();
    simulated_time_ms += 100; direct_coord.update();
    simulated_time_ms += 50;  direct_coord.update();
    simulated_time_ms += 200; direct_coord.update();
    simulated_time_ms += 100; direct_coord.update();
    simulated_time_ms += 50;  direct_coord.update();
    simulated_time_ms += 200; direct_coord.update();

    // Immediately trips to ERROR without hydraulic recovery
    EXPECT_EQ(direct_coord.get_state(), domain::MachineState::ERROR);
    EXPECT_EQ(direct_coord.get_error(), domain::MachineError::UNBALANCED_LOAD);
}

TEST_F(WashCycleCoordinatorTest, StopCycleFromErrorStateResetsToIdle)
{
    coordinator.start_cycle(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::LOW_LEVEL, false);

    // Cause fill timeout
    simulated_time_ms += 6000;
    coordinator.update();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::ERROR);

    // Stop cancels from error
    coordinator.stop_cycle();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::IDLE);
    EXPECT_EQ(coordinator.get_error(), domain::MachineError::NONE);
    EXPECT_FALSE(fill_ctrl.has_error());
}

TEST_F(WashCycleCoordinatorTest, PersistsProgressAtStepTransitionsAndClearsOnFinish)
{
    mocks::MockCyclePersistence mock_pers;
    coordinator.set_persistence(&mock_pers);

    // Step 0 (DRAIN) of SPIN_ONLY should save progress
    EXPECT_CALL(mock_pers, save_cycle_progress(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false, 0, false, false)).Times(1);
    coordinator.start_cycle(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false);

    // Step 1 (SPIN_FINAL) should save progress
    EXPECT_CALL(mock_pers, save_cycle_progress(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false, 1, false, false)).Times(1);
    coordinator.advance_step();

    // Advance past final step finishes cycle and saves finished
    EXPECT_CALL(mock_pers, save_cycle_finished(domain::WashProgram::SPIN_ONLY, domain::WaterLevel::LOW_LEVEL, false)).Times(1);
    coordinator.advance_step();

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::FINISHED);
}

TEST_F(WashCycleCoordinatorTest, StopCycleCallsSaveCycleFinished)
{
    mocks::MockCyclePersistence mock_pers;
    coordinator.set_persistence(&mock_pers);

    EXPECT_CALL(mock_pers, save_cycle_progress(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::HIGH_LEVEL, true, 0, false, false)).Times(1);
    coordinator.start_cycle(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::HIGH_LEVEL, true);

    EXPECT_CALL(mock_pers, save_cycle_finished(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::HIGH_LEVEL, true)).Times(1);
    coordinator.stop_cycle();

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::IDLE);
}

TEST_F(WashCycleCoordinatorTest, PausingAndResumingCyclePersistsPausedFlag)
{
    mocks::MockCyclePersistence mock_pers;
    coordinator.set_persistence(&mock_pers);

    EXPECT_CALL(mock_pers, save_cycle_progress(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::LOW_LEVEL, false, 0, false, false)).Times(1);
    coordinator.start_cycle(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::LOW_LEVEL, false);

    // Pause cycle should persist is_paused = true
    EXPECT_CALL(mock_pers, save_cycle_progress(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::LOW_LEVEL, false, 0, false, true)).Times(1);
    coordinator.pause_cycle();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::PAUSED);

    // Resume cycle should persist is_paused = false
    EXPECT_CALL(mock_pers, save_cycle_progress(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::LOW_LEVEL, false, 0, false, false)).Times(1);
    coordinator.resume_cycle();
    EXPECT_EQ(coordinator.get_state(), domain::MachineState::RUNNING);
}

TEST_F(WashCycleCoordinatorTest, ResumesInterruptedCycleFromWashStep)
{
    persistence::CycleSnapshot snapshot{};
    snapshot.seq_num = 5;
    snapshot.program = domain::WashProgram::NORMAL_WASH;
    snapshot.level = domain::WaterLevel::MEDIUM_LEVEL;
    snapshot.softener_enabled = true;
    snapshot.step_index = 1; // AGITATE_NORMAL
    snapshot.in_rinse_subcycle = false;
    snapshot.is_running = true;
    snapshot.is_paused = false;

    coordinator.resume_interrupted_cycle(snapshot);

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::RUNNING);
    EXPECT_EQ(coordinator.get_original_program(), domain::WashProgram::NORMAL_WASH);
    EXPECT_EQ(coordinator.get_level(), domain::WaterLevel::MEDIUM_LEVEL);
    EXPECT_TRUE(coordinator.is_softener_enabled());
    EXPECT_EQ(coordinator.get_step_index(), 1);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::AGITATE_NORMAL);
    EXPECT_TRUE(agitator.is_active());
}

TEST_F(WashCycleCoordinatorTest, ResumesInterruptedCycleInPausedStateWithoutStartingActuators)
{
    persistence::CycleSnapshot snapshot{};
    snapshot.seq_num = 8;
    snapshot.program = domain::WashProgram::HEAVY_WASH;
    snapshot.level = domain::WaterLevel::HIGH_LEVEL;
    snapshot.softener_enabled = true;
    snapshot.step_index = 2; // SOAK
    snapshot.in_rinse_subcycle = false;
    snapshot.is_running = true;
    snapshot.is_paused = true;

    coordinator.resume_interrupted_cycle(snapshot);

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::PAUSED);
    EXPECT_EQ(coordinator.get_original_program(), domain::WashProgram::HEAVY_WASH);
    EXPECT_EQ(coordinator.get_level(), domain::WaterLevel::HIGH_LEVEL);
    EXPECT_TRUE(coordinator.is_softener_enabled());
    EXPECT_EQ(coordinator.get_step_index(), 2);
    EXPECT_FALSE(fill_ctrl.is_active());
    EXPECT_FALSE(agitator.is_active());
    EXPECT_FALSE(drain_ctrl.is_active());
    EXPECT_FALSE(spin_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, ResumesInterruptedCycleFromRinseSubcycle)
{
    persistence::CycleSnapshot snapshot{};
    snapshot.seq_num = 12;
    snapshot.program = domain::WashProgram::HEAVY_WASH;
    snapshot.level = domain::WaterLevel::HIGH_LEVEL;
    snapshot.softener_enabled = true;
    snapshot.step_index = 4; // FILL_SOFTENER in rinse
    snapshot.in_rinse_subcycle = true;
    snapshot.is_running = true;
    snapshot.is_paused = false;

    coordinator.resume_interrupted_cycle(snapshot);

    EXPECT_EQ(coordinator.get_state(), domain::MachineState::RUNNING);
    EXPECT_EQ(coordinator.get_original_program(), domain::WashProgram::HEAVY_WASH);
    EXPECT_EQ(coordinator.get_level(), domain::WaterLevel::HIGH_LEVEL);
    EXPECT_TRUE(coordinator.is_softener_enabled());
    EXPECT_EQ(coordinator.get_step_index(), 4);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::FILL_SOFTENER);
    EXPECT_TRUE(fill_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, StartsFillWithBothValvesWhenSoftenerDisabled)
{
    EXPECT_CALL(mock_valve_main, turn_on()).Times(1);
    EXPECT_CALL(mock_valve_softener, turn_on()).Times(1);

    coordinator.start_cycle(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::LOW_LEVEL, false);
    EXPECT_TRUE(fill_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, StartsFillWithMainValveOnlyWhenSoftenerEnabled)
{
    EXPECT_CALL(mock_valve_main, turn_on()).Times(1);
    EXPECT_CALL(mock_valve_softener, turn_on()).Times(0);

    coordinator.start_cycle(domain::WashProgram::NORMAL_WASH, domain::WaterLevel::LOW_LEVEL, true);
    EXPECT_TRUE(fill_ctrl.is_active());
}

TEST_F(WashCycleCoordinatorTest, DoubleRinseUsesMainValveOnlyForFill1AndBothValvesForFill2)
{
    // Fill 1 (Step 0 - Wash stage or 1st rinse): Softener enabled -> softener valve must NOT turn on
    EXPECT_CALL(mock_valve_main, turn_on()).Times(1);
    EXPECT_CALL(mock_valve_softener, turn_on()).Times(0);

    coordinator.start_cycle(domain::WashProgram::RINSE_ONLY, domain::WaterLevel::LOW_LEVEL, true);
    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::FILL_MAIN);
    EXPECT_TRUE(fill_ctrl.is_active());

    // Advance to step 4 (FILL_SOFTENER): Both valves turn on
    EXPECT_CALL(mock_valve_main, turn_on()).Times(1);
    EXPECT_CALL(mock_valve_softener, turn_on()).Times(1);

    coordinator.advance_step(); // Step 1: AGITATE_NORMAL
    coordinator.advance_step(); // Step 2: DRAIN
    coordinator.advance_step(); // Step 3: SPIN_INTERMEDIATE
    coordinator.advance_step(); // Step 4: FILL_SOFTENER

    EXPECT_EQ(coordinator.get_current_step(), fsm::CycleStep::FILL_SOFTENER);
    EXPECT_TRUE(fill_ctrl.is_active());
}
