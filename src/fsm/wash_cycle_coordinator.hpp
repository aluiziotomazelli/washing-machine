#pragma once

#include <stdint.h>
#include "../domain/wash_types.hpp"
#include "../controllers/fill_controller.hpp"
#include "../controllers/agitator.hpp"
#include "../controllers/drain_controller.hpp"
#include "../controllers/spin_controller.hpp"
#include "../hal/interfaces/i_timer_hal.hpp"

namespace persistence {
class ICyclePersistence;
struct CycleSnapshot;
}

namespace fsm {

using domain::MachineError;
using domain::MachineState;
using domain::WashProgram;
using domain::WashStage;
using domain::WaterLevel;

/**
 * @brief Discrete operational step types within an orchestrated wash cycle.
 */
enum class CycleStep : uint8_t
{
    NONE = 0,
    FILL_MAIN,
    FILL_SOFTENER,
    AGITATE_GENTLE,
    AGITATE_NORMAL,
    SOAK,
    DRAIN,
    SPIN_INTERMEDIATE,
    SPIN_FINAL,
    RECOVERY_FILL,
    RECOVERY_AGITATE,
    RECOVERY_DRAIN,
    SETTLE_PAUSE,
    FINISHED
};

/**
 * @brief Configuration parameters for cycle timings (in seconds or milliseconds).
 */
struct CoordinatorConfig
{
    // Stage Pause & Settle
    uint32_t stage_settle_ms{4000}; //!< Quiet delay between major stages (4 s).

    // Wash Stage Agitation & Soak Durations (in seconds)
    uint32_t normal_wash_agitate_sec{18 * 60}; //!< Normal wash continuous agitation (18 min).
    uint32_t heavy_wash_agitate1_sec{8 * 60};  //!< Heavy wash gentle agitation (8 min).
    uint32_t heavy_wash_soak_sec{20 * 60};     //!< Heavy wash soak (20 min).
    uint32_t heavy_wash_agitate2_sec{14 * 60}; //!< Heavy wash normal agitation (14 min).

    // Rinse Stage Durations (in seconds)
    uint32_t single_rinse_agitate_sec{7 * 60};        //!< Single rinse agitation (7 min).
    uint32_t double_rinse_1_agitate_sec{5 * 60};      //!< First rinse agitation (5 min).
    uint32_t double_rinse_2_agitate_sec{2 * 60};      //!< Second rinse gentle agitation (2 min).
    uint32_t double_rinse_2_soak_sec{5 * 60};         //!< Softener soak (5 min).
    uint32_t double_rinse_2_agitate_post_sec{2 * 60}; //!< Post-soak agitation (2 min).

    // Spin Durations (in seconds)
    uint32_t intermediate_spin_sec{2 * 60}; //!< Intermediate spin after wash and rinse (2 min).
    uint32_t final_spin_sec{4 * 60};        //!< Final spin (4 min).

    // Unbalance Hydraulic Recovery
    uint8_t max_unbalance_recoveries{1}; //!< Hydraulic recovery attempts before latching an error.
    uint32_t unbalance_agitate_sec{30};  //!< Brief agitation to redistribute the load (30 s).
    domain::WaterLevel unbalance_fill_level{domain::WaterLevel::LOW_LEVEL}; //!< Minimum refill level for recovery.
};

/**
 * @class WashCycleCoordinator
 * @brief Central Finite State Machine (FSM) coordinator orchestrating laundry cycle recipes.
 *
 * Adheres strictly to the Single Responsibility Principle: does not manipulate pins or relays.
 * Instead, it delegates physical execution to dedicated atomic process controllers
 * (FillController, Agitator, DrainController, SpinController).
 */
class WashCycleCoordinator
{
public:
    /**
     * @brief Construct the cycle coordinator with its atomic controllers.
     * @param timer_hal Timer abstraction used for soak/settle timers.
     * @param fill_ctrl Fill controller used during fill steps.
     * @param agitator Agitator used during wash/rinse steps.
     * @param drain_ctrl Drain controller used during drain steps.
     * @param spin_ctrl Spin controller used during spin steps.
     * @param config Recipe durations and recovery parameters.
     * @param persistence Optional persistence manager for power-loss recovery.
     */
    WashCycleCoordinator(
        hal::ITimerHAL& timer_hal,
        controllers::FillController& fill_ctrl,
        controllers::Agitator& agitator,
        controllers::DrainController& drain_ctrl,
        controllers::SpinController& spin_ctrl,
        const CoordinatorConfig& config = CoordinatorConfig{},
        persistence::ICyclePersistence* persistence = nullptr);

    /**
     * @brief Initialize FSM state and subordinate controllers.
     */
    void init();

    /**
     * @brief Periodic non-blocking FSM update (step progression and recovery).
     */
    void update();

    // Cycle Control Interface

    /**
     * @brief Start a new wash cycle from IDLE.
     * @param program Selected wash program.
     * @param level Selected water level.
     * @param softener_enabled true to enable the softener dispenser step.
     */
    void start_cycle(WashProgram program, WaterLevel level, bool softener_enabled);

    /**
     * @brief Resume a cycle from a persisted snapshot after power loss.
     * @param snapshot Snapshot loaded from non-volatile storage.
     */
    void resume_interrupted_cycle(const persistence::CycleSnapshot& snapshot);

    /**
     * @brief Pause the running cycle and suspend all active controllers.
     */
    void pause_cycle();

    /**
     * @brief Resume a paused cycle.
     */
    void resume_cycle();

    /**
     * @brief Skip the current step and advance to the next recipe step.
     */
    void advance_step();

    /**
     * @brief Stop the running cycle and return to IDLE.
     */
    void stop_cycle();

    // Inspection Interface

    /**
     * @brief Get the current machine macro state.
     * @return Active MachineState.
     */
    MachineState get_state() const { return state_; }

    /**
     * @brief Get the current latched machine error.
     * @return Active MachineError (NONE when healthy).
     */
    MachineError get_error() const { return current_error_; }

    /**
     * @brief Get the current wash stage used for visual feedback.
     * @return Active WashStage.
     */
    WashStage get_current_stage() const { return current_stage_; }

    /**
     * @brief Get the current recipe step.
     * @return Active CycleStep.
     */
    CycleStep get_current_step() const { return current_step_; }

    /**
     * @brief Get the active wash program.
     * @return Active WashProgram.
     */
    WashProgram get_program() const { return program_; }

    /**
     * @brief Get the program chosen by the user before any diagnostic override.
     * @return Original WashProgram.
     */
    WashProgram get_original_program() const { return original_program_; }

    /**
     * @brief Get the selected water level.
     * @return Selected WaterLevel.
     */
    WaterLevel get_level() const { return level_; }

    /**
     * @brief Check whether the softener option is enabled.
     * @return true when softener dispensing is enabled.
     */
    bool is_softener_enabled() const { return softener_enabled_; }

    /**
     * @brief Get the index of the current recipe step.
     * @return Zero-based step index.
     */
    uint8_t get_step_index() const { return step_index_; }

    /**
     * @brief Get the number of unbalance recoveries already attempted.
     * @return Recovery attempt count.
     */
    uint8_t get_unbalance_recoveries() const { return unbalance_recoveries_; }

    /**
     * @brief Attach or detach the persistence manager at runtime.
     * @param persistence Persistence implementation, or nullptr to disable persistence.
     */
    void set_persistence(persistence::ICyclePersistence* persistence) { persistence_ = persistence; }

private:
    void plan_next_step();
    void execute_step(CycleStep step);
    void exit_step(CycleStep from, CycleStep to);
    void stop_active_process();
    void trigger_error(MachineError error);

    hal::ITimerHAL& timer_hal_;
    controllers::FillController& fill_ctrl_;
    controllers::Agitator& agitator_;
    controllers::DrainController& drain_ctrl_;
    controllers::SpinController& spin_ctrl_;
    CoordinatorConfig config_;
    persistence::ICyclePersistence* persistence_{nullptr};

    // Macro State
    MachineState state_{MachineState::IDLE};
    MachineError current_error_{MachineError::NONE};
    WashStage current_stage_{WashStage::IDLE};
    CycleStep current_step_{CycleStep::NONE};

    // Cycle Configuration
    WashProgram program_{WashProgram::NORMAL_WASH};
    WashProgram original_program_{WashProgram::NORMAL_WASH};
    WaterLevel level_{WaterLevel::LOW_LEVEL};
    bool softener_enabled_{false};

    // Recipe Step Tracking
    uint8_t step_index_{0};
    bool in_rinse_subcycle_{false};

    // Unbalance Recovery Tracking
    uint8_t unbalance_recoveries_{0};
    CycleStep saved_spin_step_{CycleStep::NONE};

    // Software Pause / Settle / Soak Timers
    uint32_t step_start_ms_{0};
    uint32_t soak_duration_ms_{0};
    uint32_t soak_elapsed_before_pause_ms_{0};
    bool is_soaking_{false};
    bool is_settling_{false};
};

} // namespace fsm
