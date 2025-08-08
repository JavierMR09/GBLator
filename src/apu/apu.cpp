//
// Implementation of the APU stub.
//

#include "apu/apu.h"
#include "mmu/memory.h"

namespace gblator {

APU::APU(Memory& memory) : memory_(memory) {
}

void APU::reset() {
    // Reset audio registers to power-on defaults
    // NR50, NR51, NR52 are global control registers
    memory_.writeByte(0xFF24, 0x00); // NR50
    memory_.writeByte(0xFF25, 0x00); // NR51
    memory_.writeByte(0xFF26, 0x00); // NR52: APU off
    // Clear channel registers (NR10–NR14, NR21–NR24, NR30–NR34, NR40–NR44)
    for (uint16_t addr = 0xFF10; addr <= 0xFF3F; ++addr) {
        memory_.writeByte(addr, 0x00);
    }
}

void APU::step(int /*cycles*/) {
    // Stub: A real APU would generate sound samples here.
    // This stub does nothing but could process envelopes and length timers.
}

} // namespace gblator