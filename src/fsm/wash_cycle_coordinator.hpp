#pragma once

#include <stdint.h>
#include "../domain/wash_types.hpp"
#include "../controllers/fill_controller.hpp"
#include "../controllers/agitator.hpp"
#include "../controllers/drain_controller.hpp"
#include "../controllers/spin_controller.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace fsm {

using domain::MachineState;
using domain::WashProgram;
using domain::WashStage;
using domain::WaterLevel;

/**
 * @brief Discrete operational step types within an orchestrated wash cycle.
 */
enum class CycleStep : uint8_t {
    NONE = 0,
    FILL_MAIN,
    FILL_SOFTENER,
    AGITATE_GENTLE,
    AGITATE_NORMAL,
    SOAK,
    DRAIN,
    SPIN_INTERMEDIATE,
    SPIN_FINAL,
    SETTLE_PAUSE,
    FINISHED
};

/**
 * @brief Configuration parameters for cycle timings (in seconds or milliseconds).
 */
struct CoordinatorConfig {
    // Stage Pause & Settle
    uint32_t stage_settle_ms{4000};        // 4s quiet delay between major stages

    // Wash Stage Agitation & Soak Durations (in seconds)
    uint32_t normal_wash_agitate_sec{18 * 60};     // 18 min continuous agitation
    uint32_t heavy_wash_agitate1_sec{8 * 60};      // 8 min gentle agitation
    uint32_t heavy_wash_soak_sec{20 * 60};         // 20 min soak
    uint32_t heavy_wash_agitate2_sec{14 * 60};     // 14 min normal agitation

    // Rinse Stage Durations (in seconds)
    uint32_t single_rinse_agitate_sec{7 * 60};     // 7 min single rinse
    uint32_t double_rinse_1_agitate_sec{5 * 60};   // 5 min 1st rinse
    uint32_t double_rinse_interm_spin_sec{2 * 60}; // 2 min intermediate spin
    uint32_t double_rinse_2_agitate_sec{2 * 60};   // 2 min gentle rinse with softener
    uint32_t double_rinse_2_soak_sec{5 * 60};      // 5 min softener soak
    uint32_t double_rinse_2_agitate_post_sec{2 * 60}; // 2 min post-soak agitation

    // Final Spin Duration (in seconds)
    uint32_t final_spin_sec{4 * 60};               // 4 min final spin
};

/**
 * @class WashCycleCoordinator
 * @brief Central Finite State Machine (FSM) coordinator orchestrating laundry cycle recipes.
 * 
 * Adheres strictly to the Single Responsibility Principle: does not manipulate pins or relays.
 * Instead, it delegates physical execution to dedicated atomic process controllers
 * (FillController, Agitator, DrainController, SpinController).
 */
class WashCycleCoordinator {
public:
    WashCycleCoordinator(
        hal::ITimerHAL& timer_hal,
        controllers::FillController& fill_ctrl,
        controllers::Agitator& agitator,
        controllers::DrainController& drain_ctrl,
        controllers::SpinController& spin_ctrl,
        const CoordinatorConfig& config = CoordinatorConfig{}
    );

    void init();
    void update();

    // Cycle Control Interface
    void start_cycle(WashProgram program, WaterLevel level, bool softener_enabled);
    void pause_cycle();
    void resume_cycle();
    void advance_step();
    void stop_cycle();

    // Inspection Interface
    MachineState get_state() const { return state_; }
    WashStage get_current_stage() const { return current_stage_; }
    CycleStep get_current_step() const { return current_step_; }
    WashProgram get_program() const { return program_; }
    WaterLevel get_level() const { return level_; }
    bool is_softener_enabled() const { return softener_enabled_; }
    uint8_t get_step_index() const { return step_index_; }

private:
    void plan_next_step();
    void execute_step(CycleStep step);
    void stop_active_process();
    void trigger_error();

    hal::ITimerHAL& timer_hal_;
    controllers::FillController& fill_ctrl_;
    controllers::Agitator& agitator_;
    controllers::DrainController& drain_ctrl_;
    controllers::SpinController& spin_ctrl_;
    CoordinatorConfig config_;

    // Macro State
    MachineState state_{MachineState::IDLE};
    WashStage current_stage_{WashStage::IDLE};
    CycleStep current_step_{CycleStep::NONE};

    // Cycle Configuration
    WashProgram program_{WashProgram::NORMAL_WASH};
    WaterLevel level_{WaterLevel::LOW_LEVEL};
    bool softener_enabled_{false};

    // Recipe Step Tracking
    uint8_t step_index_{0};
    bool in_rinse_subcycle_{false};

    // Software Pause / Settle / Soak Timers
    uint32_t step_start_ms_{0};
    uint32_t soak_duration_ms_{0};
    uint32_t soak_elapsed_before_pause_ms_{0};
    bool is_soaking_{false};
    bool is_settling_{false};
};

} // namespace fsm
