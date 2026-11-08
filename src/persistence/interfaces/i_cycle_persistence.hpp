#pragma once

#include <stdint.h>
#include "../../domain/wash_types.hpp"

namespace persistence {

using domain::WashProgram;
using domain::WaterLevel;

/**
 * @struct CycleSnapshot
 * @brief Represents the persisted operational state of the washing machine.
 */
struct CycleSnapshot {
    uint16_t seq_num{0};                //!< Monotonic sequence number used to pick the newest record.
    WashProgram program{WashProgram::NORMAL_WASH}; //!< Wash program being executed.
    WaterLevel level{WaterLevel::LOW_LEVEL};       //!< Selected water level.
    uint8_t step_index{0};              //!< Index of the active recipe step.
    bool in_rinse_subcycle{false};      //!< true while executing the second rinse subcycle.
    bool softener_enabled{false};       //!< true when the softener option is enabled.
    bool is_running{false};             //!< true when a cycle was running at save time.
    bool is_paused{false};              //!< true when the cycle was paused at save time.
    uint8_t checksum{0};                //!< CRC-8 over the preceding fields.
};

/**
 * @interface ICyclePersistence
 * @brief Abstract interface for machine state persistence and power-loss recovery.
 */
class ICyclePersistence {
public:
    virtual ~ICyclePersistence() = default;

    /**
     * @brief Load the most recent valid snapshot from non-volatile storage.
     * @param out_snapshot Output structure to receive the loaded state.
     * @return true if a valid snapshot was found; false if storage is blank/corrupted.
     */
    virtual bool load_latest(CycleSnapshot& out_snapshot) = 0;

    /**
     * @brief Persist user program/level preferences during IDLE selection.
     * @param program Selected wash program.
     * @param level Selected water level.
     * @param softener_enabled true when the softener option is enabled.
     */
    virtual void save_user_selection(WashProgram program, WaterLevel level, bool softener_enabled) = 0;

    /**
     * @brief Persist active cycle execution progress across step transitions and pauses.
     * @param program Active wash program.
     * @param level Selected water level.
     * @param softener_enabled true when the softener option is enabled.
     * @param step_index Index of the current recipe step.
     * @param in_rinse_subcycle true while executing the second rinse subcycle.
     * @param is_paused true when the cycle is suspended by the user.
     */
    virtual void save_cycle_progress(
        WashProgram program,
        WaterLevel level,
        bool softener_enabled,
        uint8_t step_index,
        bool in_rinse_subcycle,
        bool is_paused = false
    ) = 0;

    /**
     * @brief Persist cycle completion or user stop (clears running flag).
     * @param program Wash program that was executed.
     * @param level Water level that was used.
     * @param softener_enabled true when the softener option was enabled.
     */
    virtual void save_cycle_finished(WashProgram program, WaterLevel level, bool softener_enabled) = 0;
};

} // namespace persistence
