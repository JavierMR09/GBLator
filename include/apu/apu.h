//
// Part of the GBLator project.
//
// This header declares a minimal Audio Processing Unit (APU) stub. The
// Game Boy APU is complex, featuring four channels and many registers.
// This implementation merely stores the audio registers and provides a
// step function placeholder. Full sound emulation is beyond the scope
// of this skeleton.

#ifndef GBLATOR_APU_H
#define GBLATOR_APU_H

#include <cstdint>

namespace gblator {

class Memory;

/**
 * @brief Minimal stub for the Game Boy’s audio hardware.
 *
 * The APU class stores the audio register values and updates nothing
 * else. Real audio generation would require mixing channel outputs and
 * producing PCM samples, which is out of scope for this demonstration.
 */
class APU {
public:
    explicit APU(Memory& memory);
    /** Reset audio registers and state. */
    void reset();
    /** Step the APU by the given number of CPU cycles. */
    void step(int cycles);
private:
    Memory& memory_;
};

} // namespace gblator

#endif // GBLATOR_APU_H