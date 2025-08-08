//
// Implementation of the Timer class.
//

#include "utils/timer.h"
#include "mmu/memory.h"

namespace gblator {

Timer::Timer(Memory& memory) : memory_(memory), divCounter_(0), timaCounter_(0) {
}

void Timer::reset() {
    divCounter_ = 0;
    timaCounter_ = 0;
    // Reset DIV and TIMA/TMA/TAC registers
    memory_.writeByte(0xFF04, 0x00);
    memory_.writeByte(0xFF05, 0x00);
    memory_.writeByte(0xFF06, 0x00);
    memory_.writeByte(0xFF07, 0x00);
}

int Timer::timerPeriod() const {
    uint8_t tac = memory_.readByte(0xFF07);
    if ((tac & 0x04) == 0) {
        // Timer disabled
        return 0;
    }
    switch (tac & 0x03) {
    case 0: return 1024;  // 4096 Hz → 1024 CPU cycles per increment
    case 1: return 16;    // 262144 Hz → 16 CPU cycles per increment
    case 2: return 64;    // 65536 Hz → 64 CPU cycles per increment
    case 3: return 256;   // 16384 Hz → 256 CPU cycles per increment
    default: return 1024;
    }
}

void Timer::step(int cycles) {
    // Update divider; increments at 16384 Hz => 256 cycles per increment
    divCounter_ += cycles;
    while (divCounter_ >= 256) {
        divCounter_ -= 256;
        uint8_t div = memory_.readByte(0xFF04);
        div = static_cast<uint8_t>(div + 1);
        memory_.writeByte(0xFF04, div);
    }

    // Update TIMA if timer enabled
    int period = timerPeriod();
    if (period > 0) {
        timaCounter_ += cycles;
        while (timaCounter_ >= period) {
            timaCounter_ -= period;
            uint8_t tima = memory_.readByte(0xFF05);
            if (tima == 0xFF) {
                // Overflow: reload from TMA and request timer interrupt
                uint8_t tma = memory_.readByte(0xFF06);
                memory_.writeByte(0xFF05, tma);
                // Request timer interrupt (IF bit 2)
                uint8_t iflags = memory_.readByte(0xFF0F);
                iflags |= 0x04;
                memory_.writeByte(0xFF0F, iflags);
            } else {
                tima = static_cast<uint8_t>(tima + 1);
                memory_.writeByte(0xFF05, tima);
            }
        }
    }
}

} // namespace gblator