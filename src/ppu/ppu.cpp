//
// Implementation of the PPU class.
//

#include "ppu/ppu.h"
#include "mmu/memory.h"

namespace gblator {

PPU::PPU(Memory& memory)
    : memory_(memory), dotCounter_(0), ly_(0), mode_(2), vblankTriggered_(false) {
}

void PPU::reset() {
    dotCounter_ = 0;
    ly_ = 0;
    mode_ = 2; // Mode 2 (OAM search) at start of frame
    vblankTriggered_ = false;
    // Write initial LY value to memory
    memory_.writeByte(0xFF44, ly_);
    updateSTAT();
}

void PPU::updateSTAT() {
    // Read current STAT register
    uint8_t stat = memory_.readByte(0xFF41);
    // Set the lower two bits to the current PPU mode
    stat = static_cast<uint8_t>((stat & 0xFC) | (mode_ & 0x03));
    // Set or clear the LYC=LY flag (bit 2)
    uint8_t lyc = memory_.readByte(0xFF45);
    if (lyc == ly_) {
        stat |= 0x04;
    } else {
        stat &= static_cast<uint8_t>(~0x04);
    }
    memory_.writeByte(0xFF41, stat);
}

void PPU::step(int cycles) {
    // Convert CPU cycles to PPU dots; 1 CPU cycle = 4 dots at single speed
    int dots = cycles * 4;
    // If LCD is disabled, reset LY and remain in mode 0
    uint8_t lcdc = memory_.readByte(0xFF40);
    if ((lcdc & 0x80) == 0) {
        // LCD & PPU disabled
        ly_ = 0;
        dotCounter_ = 0;
        mode_ = 0;
        vblankTriggered_ = false;
        memory_.writeByte(0xFF44, ly_);
        updateSTAT();
        return;
    }
    dotCounter_ += dots;
    // Process dots, potentially advancing multiple scanlines
    while (dotCounter_ >= 456) {
        dotCounter_ -= 456;
        ly_++;
        if (ly_ == 144) {
            // Enter VBlank
            mode_ = 1;
            // Request VBlank interrupt if not already triggered
            if (!vblankTriggered_) {
                uint8_t iflags = memory_.readByte(0xFF0F);
                iflags |= 0x01; // Bit 0: VBlank interrupt
                memory_.writeByte(0xFF0F, iflags);
                vblankTriggered_ = true;
            }
        } else if (ly_ > 153) {
            // Restart frame
            ly_ = 0;
            mode_ = 2;
            vblankTriggered_ = false;
        } else if (ly_ < 144) {
            // Visible scanlines start in mode 2
            mode_ = 2;
        }
        // Update LY register each scanline increment
        memory_.writeByte(0xFF44, ly_);
        updateSTAT();
    }
    // Determine mode based on dotCounter for current scanline (0-143 only)
    if (ly_ < 144) {
        // Modes: 2 (0-79 dots), 3 (80-251+ dots), 0 (rest)
        if (dotCounter_ < 80) {
            mode_ = 2;
        } else if (dotCounter_ < 80 + 172) {
            mode_ = 3;
        } else {
            mode_ = 0;
        }
    } else {
        // VBlank
        mode_ = 1;
    }
    updateSTAT();
}

} // namespace gblator