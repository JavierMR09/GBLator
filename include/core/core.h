//
// Part of the GBLator project.
//
// This header declares a GameBoy class that ties together the CPU,
// memory, PPU, timer, APU, and joypad. It provides a simple run loop
// for emulation.

#ifndef GBLATOR_CORE_H
#define GBLATOR_CORE_H

#include <string>

namespace gblator {

class Memory;
class CPU;
class PPU;
class Timer;
class APU;
class Joypad;

/**
 * @brief Represents an instance of the Game Boy console.
 *
 * This class owns the major subsystems and provides methods to load
 * a ROM and execute it, stepping all components in sync.
 */
class GameBoy {
public:
    GameBoy();
    ~GameBoy();

    /**
     * Load a ROM into memory.
     * @param filepath Path to the ROM file
     * @return true on success
     */
    bool loadROM(const std::string& filepath);
    /** Reset all components to initial state. */
    void reset();
    /**
     * Run the emulator for a number of CPU instructions.
     *
     * @param instructionCount Number of instructions to execute
     */
    void run(int instructionCount);
    /** Access underlying memory. */
    Memory& memory();
    CPU& cpu();
    PPU& ppu();
    Timer& timer();
    APU& apu();
    Joypad& joypad();
private:
    Memory* memory_;
    CPU* cpu_;
    PPU* ppu_;
    Timer* timer_;
    APU* apu_;
    Joypad* joypad_;
};

} // namespace gblator

#endif // GBLATOR_GAMEBOY_H