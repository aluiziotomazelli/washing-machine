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
    uint16_t seq_num{0};
    WashProgram program{WashProgram::NORMAL_WASH};
    WaterLevel level{WaterLevel::LOW_LEVEL};
    uint8_t step_index{0};
    bool in_rinse_subcycle{false};
    bool softener_enabled{false};
    bool is_running{false};
    bool is_paused{false};
    uint8_t checksum{0};
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
     */
    virtual void save_user_selection(WashProgram program, WaterLevel level, bool softener_enabled) = 0;

    /**
     * @brief Persist active cycle execution progress across step transitions and pauses.
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
     */
    virtual void save_cycle_finished(WashProgram program, WaterLevel level, bool softener_enabled) = 0;
};

} // namespace persistence
