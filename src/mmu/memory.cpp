//
// Implementation of the Memory class defined in memory.h
//

#include "mmu/memory.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>

namespace gblator {

Memory::Memory()
    : romBankLow_(1), romBankHigh_(0), bankingMode_(0), ramEnabled_(false),
      vramBank_(0), wramBank_(1), cartType_(0), numRomBanks_(0), numRamBanks_(0) {
    // Allocate and zero-initialise RAM regions
    vram0_.assign(0x2000, 0);  // 8 KiB VRAM bank 0
    vram1_.assign(0x2000, 0);  // 8 KiB VRAM bank 1 (used only in CGB)
    wram_.assign(8 * 0x1000, 0);  // 8 banks of 4 KiB work RAM
    eram_.clear();                // External RAM allocated when ROM is loaded
    oam_.assign(0xA0, 0);         // 160 bytes of OAM
    std::fill(std::begin(ioRegisters_), std::end(ioRegisters_), 0);
    hram_.assign(0x7F, 0);        // 127 bytes of HRAM
    ieRegister_ = 0;
}

bool Memory::loadROM(const std::string &filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    // Load entire ROM into memory
    romData_.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    if (romData_.empty()) {
        return false;
    }
    // Ensure there is at least a header to read cartridge info
    if (romData_.size() >= 0x150) {
        cartType_ = romData_[0x0147];
        uint8_t romSizeCode = romData_[0x0148];
        uint8_t ramSizeCode = romData_[0x0149];
        // Determine number of ROM banks using the ROM size code
        switch (romSizeCode) {
        case 0x00:
            numRomBanks_ = 2;
            break; // 32 KiB
        case 0x01:
            numRomBanks_ = 4;
            break; // 64 KiB
        case 0x02:
            numRomBanks_ = 8;
            break; // 128 KiB
        case 0x03:
            numRomBanks_ = 16;
            break; // 256 KiB
        case 0x04:
            numRomBanks_ = 32;
            break; // 512 KiB
        case 0x05:
            numRomBanks_ = 64;
            break; // 1 MiB
        case 0x06:
            numRomBanks_ = 128;
            break; // 2 MiB
        case 0x07:
            numRomBanks_ = 256;
            break; // 4 MiB
        case 0x08:
            numRomBanks_ = 512;
            break; // 8 MiB
        case 0x52:
            numRomBanks_ = 72;
            break; // 1.1 MiB (unofficial)
        case 0x53:
            numRomBanks_ = 80;
            break; // 1.2 MiB (unofficial)
        case 0x54:
            numRomBanks_ = 96;
            break; // 1.5 MiB (unofficial)
        default:
            // Fallback: compute from file size (16 KiB per bank)
            numRomBanks_ = romData_.size() / 0x4000;
            break;
        }
        if (numRomBanks_ == 0) {
            numRomBanks_ = 1;
        }
        // Determine number of external RAM banks using the RAM size code
        switch (ramSizeCode) {
        case 0x00:
            numRamBanks_ = 0;
            break; // No RAM
        case 0x01:
            numRamBanks_ = 0;
            break; // Unused (2 KiB, not used in real carts)
        case 0x02:
            numRamBanks_ = 1;
            break; // 8 KiB RAM
        case 0x03:
            numRamBanks_ = 4;
            break; // 32 KiB RAM (4 banks of 8 KiB)
        case 0x04:
            numRamBanks_ = 16;
            break; // 128 KiB RAM (16 banks of 8 KiB)
        case 0x05:
            numRamBanks_ = 8;
            break; // 64 KiB RAM (8 banks of 8 KiB)
        default:
            numRamBanks_ = 0;
            break;
        }
    } else {
        // If header is missing, assume simplest ROM: 2 banks, no external RAM
        cartType_ = 0x00;
        numRomBanks_ = romData_.size() / 0x4000;
        if (numRomBanks_ == 0) numRomBanks_ = 1;
        numRamBanks_ = 0;
    }
    // Allocate external RAM (if any)
    eram_.assign(numRamBanks_ * 0x2000, 0);
    // Reset state to initial values
    reset();
    return true;
}

void Memory::reset() {
    // Clear all RAM regions and registers
    std::fill(vram0_.begin(), vram0_.end(), 0);
    std::fill(vram1_.begin(), vram1_.end(), 0);
    std::fill(wram_.begin(), wram_.end(), 0);
    std::fill(eram_.begin(), eram_.end(), 0);
    std::fill(oam_.begin(), oam_.end(), 0);
    std::fill(std::begin(ioRegisters_), std::end(ioRegisters_), 0);
    std::fill(hram_.begin(), hram_.end(), 0);
    ieRegister_ = 0;
    // Reset bank registers
    romBankLow_ = 1;
    romBankHigh_ = 0;
    bankingMode_ = 0;
    ramEnabled_ = false;
    vramBank_ = 0;
    wramBank_ = 1;
}

uint8_t Memory::currentROMBank() const {
    // Combine lower 5 bits and upper 2 bits to form a bank index
    uint8_t bank = (romBankHigh_ << 5) | (romBankLow_ & 0x1F);
    // Prevent selecting bank 0 for the switchable region if more than one bank exists
    if (bank == 0 && numRomBanks_ > 1) {
        bank = 1;
    }
    // Mask to the available number of banks
    bank %= static_cast<uint8_t>(numRomBanks_);
    return bank;
}

uint8_t Memory::currentRAMBank() const {
    if (bankingMode_ == 0) {
        return 0;
    }
    if (numRamBanks_ == 0) {
        return 0;
    }
    uint8_t bank = romBankHigh_ & 0x03;
    bank %= static_cast<uint8_t>(numRamBanks_);
    return bank;
}

uint8_t Memory::readByte(uint16_t address) const {
    if (address < 0x4000) {
        // Fixed ROM bank (00)
        size_t idx = address;
        if (idx < romData_.size()) {
            return romData_[idx];
        }
        return 0xFF;
    } else if (address < 0x8000) {
        // Switchable ROM bank
        uint8_t bank = currentROMBank();
        size_t offset = (bank * 0x4000) + (address - 0x4000);
        if (offset < romData_.size()) {
            return romData_[offset];
        }
        return 0xFF;
    } else if (address < 0xA000) {
        // VRAM (8000–9FFF)
        uint16_t offset = address - 0x8000;
        if (vramBank_ == 0) {
            return vram0_[offset];
        } else {
            return vram1_[offset];
        }
    } else if (address < 0xC000) {
        // External RAM (A000–BFFF)
        if (numRamBanks_ == 0 || !ramEnabled_) {
            return 0xFF;
        }
        uint8_t bank = currentRAMBank();
        size_t offset = (bank * 0x2000) + (address - 0xA000);
        if (offset < eram_.size()) {
            return eram_[offset];
        }
        return 0xFF;
    } else if (address < 0xD000) {
        // Work RAM bank 0 (C000–CFFF)
        size_t offset = address - 0xC000;
        return wram_[offset];
    } else if (address < 0xE000) {
        // Work RAM bank 1–7 (D000–DFFF)
        uint8_t bank = wramBank_;
        if (bank == 0) {
            bank = 1;
        }
        bank &= 0x07;
        size_t offset = (bank * 0x1000) + (address - 0xD000);
        return wram_[offset];
    } else if (address < 0xFE00) {
        // Echo RAM (E000–FDFF) mirrors C000–DDFF
        return readByte(address - 0x2000);
    } else if (address < 0xFEA0) {
        // OAM (FE00–FE9F)
        uint16_t offset = address - 0xFE00;
        return oam_[offset];
    } else if (address < 0xFF00) {
        // FEA0–FEFF: Not usable
        return 0xFF;
    } else if (address < 0xFF80) {
        // I/O registers (FF00–FF7F)
        uint8_t index = static_cast<uint8_t>(address - 0xFF00);
        // Some registers return certain bits set or cleared
        switch (address) {
        case 0xFF4F: // VBK - VRAM bank select
            // Bit 0 holds the bank number; upper bits often return 1
            return static_cast<uint8_t>(0xFE | (vramBank_ & 0x01));
        case 0xFF70: // SVBK - WRAM bank select
            // Bits 0-2 hold bank (0->1); upper bits usually return 0xF8
            return static_cast<uint8_t>(0xF8 | (wramBank_ & 0x07));
        default:
            return ioRegisters_[index];
        }
    } else if (address < 0xFFFF) {
        // High RAM (FF80–FFFE)
        uint16_t offset = address - 0xFF80;
        return hram_[offset];
    } else {
        // Interrupt Enable register
        return ieRegister_;
    }
}

void Memory::writeByte(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        // 0000–1FFF: RAM enable (MBC1). Only lower 4 bits are significant.
        // For ROM only carts, this has no effect.
        if (cartType_ == 0x01 || cartType_ == 0x02 || cartType_ == 0x03) {
            ramEnabled_ = ((value & 0x0F) == 0x0A);
        }
        // else ignore
    } else if (address < 0x4000) {
        // 2000–3FFF: ROM bank number (MBC1)
        if (cartType_ == 0x01 || cartType_ == 0x02 || cartType_ == 0x03) {
            romBankLow_ = value & 0x1F;
            // Bank number 0 maps to 1
            if ((romBankLow_ & 0x1F) == 0) {
                romBankLow_ = 1;
            }
        }
        // else ignore
    } else if (address < 0x6000) {
        // 4000–5FFF: RAM bank number or upper bits of ROM bank number (MBC1)
        if (cartType_ == 0x01 || cartType_ == 0x02 || cartType_ == 0x03) {
            romBankHigh_ = value & 0x03;
        }
    } else if (address < 0x8000) {
        // 6000–7FFF: Banking mode select (MBC1)
        if (cartType_ == 0x01 || cartType_ == 0x02 || cartType_ == 0x03) {
            bankingMode_ = (value & 0x01);
        }
    } else if (address < 0xA000) {
        // 8000–9FFF: VRAM
        uint16_t offset = address - 0x8000;
        if (vramBank_ == 0) {
            vram0_[offset] = value;
        } else {
            vram1_[offset] = value;
        }
    } else if (address < 0xC000) {
        // A000–BFFF: External RAM
        if (numRamBanks_ != 0 && ramEnabled_) {
            uint8_t bank = currentRAMBank();
            size_t offset = (bank * 0x2000) + (address - 0xA000);
            if (offset < eram_.size()) {
                eram_[offset] = value;
            }
        }
        // else ignore writes
    } else if (address < 0xD000) {
        // C000–CFFF: Work RAM bank 0
        size_t offset = address - 0xC000;
        wram_[offset] = value;
    } else if (address < 0xE000) {
        // D000–DFFF: Work RAM bank 1–7
        uint8_t bank = wramBank_;
        if (bank == 0) {
            bank = 1;
        }
        bank &= 0x07;
        size_t offset = (bank * 0x1000) + (address - 0xD000);
        wram_[offset] = value;
    } else if (address < 0xFE00) {
        // E000–FDFF: Echo RAM (mirror of C000–DDFF)
        writeByte(address - 0x2000, value);
    } else if (address < 0xFEA0) {
        // FE00–FE9F: OAM
        uint16_t offset = address - 0xFE00;
        if (offset < oam_.size()) {
            oam_[offset] = value;
        }
    } else if (address < 0xFF00) {
        // FEA0–FEFF: Not usable; writes ignored
        return;
    } else if (address < 0xFF80) {
        // FF00–FF7F: I/O registers
        uint8_t index = static_cast<uint8_t>(address - 0xFF00);
        switch (address) {
        case 0xFF04:
            // DIV register: writing resets to 0 regardless of value
            resetDIV();
            break;
        case 0xFF00:
            // Joypad input (P1): lower 4 bits reflect button states; just store for now
            ioRegisters_[index] = value;
            break;
        case 0xFF46: {
            // DMA transfer: writing a byte triggers a transfer from
            // (value << 8) to OAM. Copy 160 bytes.
            uint16_t source = static_cast<uint16_t>(value) << 8;
            for (uint16_t i = 0; i < 0xA0; ++i) {
                uint16_t srcAddr = source + i;
                oam_[i] = readByte(srcAddr);
            }
            ioRegisters_[index] = value;
            break;
        }
        case 0xFF4F:
            // VBK: VRAM bank select
            vramBank_ = value & 0x01;
            ioRegisters_[index] = value;
            break;
        case 0xFF70:
            // SVBK: WRAM bank select; 0->1
            wramBank_ = value & 0x07;
            if (wramBank_ == 0) {
                wramBank_ = 1;
            }
            ioRegisters_[index] = value;
            break;
        case 0xFF50:
            // Boot ROM disable; just store value
            ioRegisters_[index] = value;
            break;
        default:
            ioRegisters_[index] = value;
            break;
        }
    } else if (address < 0xFFFF) {
        // FF80–FFFE: High RAM (HRAM)
        uint16_t offset = address - 0xFF80;
        if (offset < hram_.size()) {
            hram_[offset] = value;
        }
    } else {
        // FFFF: Interrupt enable register
        ieRegister_ = value;
    }
}

// -----------------------------------------------------------------------------
// Divider register helpers
//
// The divider register (FF04) increments internally at 16384 Hz (every 256 CPU
// cycles) and resets to zero when written to by the CPU【487600738692240†L125-L171】.
// The timer calls incrementDIV() to update the register without triggering
// a reset. When the CPU writes to FF04 via Memory::writeByte, resetDIV() is
// invoked instead.

void Memory::incrementDIV() {
    // The DIV register is mapped at FF04; its index in ioRegisters_ is 0x04.
    ioRegisters_[0x04] = static_cast<uint8_t>(ioRegisters_[0x04] + 1);
}

void Memory::resetDIV() {
    // Reset the DIV register to zero. This is called when the CPU writes to FF04.
    ioRegisters_[0x04] = 0;
}
} // namespace gblator