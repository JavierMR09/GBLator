//
// Part of the GBLator project: Nintendo Game Boy emulator.
//
// This header declares a very simple PPU (Pixel Processing Unit) class which
// manages the LCD state machine for the Game Boy. It tracks scanline timing,
// PPU modes (HBlank, VBlank, OAM search, pixel transfer), and updates
// relevant memory‑mapped registers (LY and STAT). It does not implement
// actual pixel rendering; instead, it provides the framework to add a
// renderer later. The timings and mode durations are based on the Pan Docs
// description of PPU modes【602189112438222†L141-L160】.

#ifndef GBLATOR_PPU_H
#define GBLATOR_PPU_H

#include <cstdint>

namespace gblator {

class Memory;

/**
 * @brief Simple Pixel Processing Unit emulation.
 *
 * The PPU cycles through four modes each scanline: OAM search (mode 2),
 * pixel transfer (mode 3), HBlank (mode 0), and VBlank (mode 1) across 154
 * scanlines【602189112438222†L141-L149】. This class updates the mode and LY
 * registers accordingly, requests VBlank interrupts at the start of VBlank,
 * and updates the STAT register’s mode bits and LYC compare flag. It expects
 * to be stepped once per CPU instruction with the number of cycles taken by
 * that instruction.
 */
class PPU {
public:
    /**
     * @brief Construct a new PPU attached to a Memory instance.
     *
     * @param memory Reference to the emulator's memory. The PPU will update
     * the LY and STAT registers and request interrupts via memory.
     */
    explicit PPU(Memory& memory);

    /**
     * @brief Reset the PPU state to initial values.
     */
    void reset();

    /**
     * @brief Step the PPU by the given number of CPU cycles.
     *
     * Each CPU instruction advances the PPU by a certain number of cycles.
     * This method converts CPU cycles to PPU dots (4 dots per CPU cycle on
     * single‑speed systems) and updates the PPU state accordingly.
     *
     * @param cycles Number of CPU cycles that have elapsed
     */
    void step(int cycles);

private:
    Memory& memory_;
    int dotCounter_;  ///< Current dot within the current scanline (0–455)
    uint8_t ly_;      ///< Current scanline (0–153)
    uint8_t mode_;    ///< Current PPU mode (0–3)
    bool vblankTriggered_; ///< Whether the VBlank interrupt has been triggered this frame

    /**
     * @brief Update the STAT register’s mode bits and LYC=LY flag.
     */
    void updateSTAT();
};

} // namespace gblator

#endif // GBLATOR_PPU_H