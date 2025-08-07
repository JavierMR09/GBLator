//
// Copyright (c) 2025.
//
// This file is part of the GBLator project, a Nintendo Game Boy
// emulator written in C++ as a learning exercise. The Memory class
// implemented here models the Game Boy’s entire 16‑bit address space.
// It handles ROM banking via a simple MBC1 implementation, VRAM bank
// switching, external RAM, work RAM (including echo RAM), OAM, I/O
// registers, HRAM and the interrupt enable register. See the Pan Docs
// memory map【823076487962412†L119-L136】 and MBC1 documentation【308000491253238†L190-L214】 for details on how the
// underlying hardware behaves.

#ifndef GBLATOR_MEMORY_H
#define GBLATOR_MEMORY_H

#include <cstdint>
#include <string>
#include <vector>

namespace gblator {

/**
 * @brief Represents the Game Boy's memory and implements address decoding.
 *
 * The Game Boy has a 16‑bit address bus which maps to various regions:
 * - 0000–3FFF: Fixed ROM bank 0【823076487962412†L119-L131】
 * - 4000–7FFF: Switchable ROM bank【823076487962412†L123-L130】
 * - 8000–9FFF: Video RAM (VRAM), 8 KiB with bank switching on CGB【823076487962412†L123-L136】
 * - A000–BFFF: External RAM on the cartridge【823076487962412†L123-L135】
 * - C000–CFFF: Work RAM bank 0
 * - D000–DFFF: Work RAM bank 1–7 (CGB)【823076487962412†L123-L135】
 * - E000–FDFF: Echo RAM (mirror of C000–DDFF)【823076487962412†L130-L131】
 * - FE00–FE9F: Object Attribute Memory (OAM)【823076487962412†L132-L136】
 * - FEA0–FEFF: Not usable
 * - FF00–FF7F: I/O registers【823076487962412†L134-L135】
 * - FF80–FFFE: High RAM (HRAM)
 * - FFFF: Interrupt Enable register
 *
 * This class handles those regions, provides basic MBC1 support for
 * switching ROM and RAM banks, and exposes methods to read and write
 * individual bytes. Additional cartridge types and detailed hardware
 * behaviour can be implemented later.
 */
class Memory {
public:
    /**
     * @brief Construct a new Memory instance.
     *
     * Initializes all memory regions to zero and sets default bank
     * registers. ROM is loaded separately via loadROM().
     */
    Memory();

    /**
     * @brief Read a byte from the given address.
     *
     * This method decodes the address and returns the value stored
     * in the appropriate memory region. Reads from the not‑usable
     * region return 0xFF.
     *
     * @param address 16‑bit address to read from
     * @return The byte stored at the specified address
     */
    uint8_t readByte(uint16_t address) const;

    /**
     * @brief Write a byte to the given address.
     *
     * Writes to the ROM address range are interpreted as MBC control
     * commands. Writes to the not‑usable region are ignored. Special
     * registers (e.g. VRAM/WRAM bank select) are handled here.
     *
     * @param address 16‑bit address to write to
     * @param value The byte to store at the specified address
     */
    void writeByte(uint16_t address, uint8_t value);

    /**
     * @brief Load a ROM file into memory.
     *
     * Reads the entire file into the internal ROM buffer, parses
     * the cartridge header to determine MBC type and RAM size, and
     * allocates external RAM accordingly. The first 32 KiB (two
     * banks) of ROM are accessible at 0000–7FFF. Additional banks
     * may be selected via the MBC registers when present.
     *
     * @param filepath Path to the ROM file on disk
     * @return true on success, false if the file could not be opened
     */
    bool loadROM(const std::string &filepath);

    /**
     * @brief Reset memory to initial state.
     *
     * Clears all RAM regions, resets bank registers and disables
     * external RAM. ROM contents are preserved.
     */
    void reset();

private:
    // Helpers to compute current ROM bank and RAM bank based on MBC
    uint8_t currentROMBank() const;
    uint8_t currentRAMBank() const;

    // Cartridge and memory configuration
    std::vector<uint8_t> romData_;      ///< Entire ROM data loaded from file
    std::vector<uint8_t> eram_;         ///< External RAM (cartridge RAM)
    std::vector<uint8_t> wram_;         ///< Work RAM (8 banks of 4 KiB each)
    std::vector<uint8_t> vram0_;        ///< VRAM bank 0 (8 KiB)
    std::vector<uint8_t> vram1_;        ///< VRAM bank 1 (8 KiB, CGB only)
    std::vector<uint8_t> oam_;          ///< Object Attribute Memory (160 bytes)
    uint8_t ioRegisters_[0x80];         ///< I/O registers FF00–FF7F
    std::vector<uint8_t> hram_;         ///< High RAM (127 bytes)
    uint8_t ieRegister_;                ///< Interrupt Enable register at FFFF

    // MBC1 state
    uint8_t romBankLow_;                ///< 5‑bit lower ROM bank register (00→01 translation applies)
    uint8_t romBankHigh_;               ///< 2‑bit upper ROM bank register / RAM bank register
    bool bankingMode_;                  ///< MBC1 banking mode (0 = simple, 1 = advanced)
    bool ramEnabled_;                   ///< Whether external RAM is enabled

    // Additional bank selectors
    uint8_t vramBank_;                  ///< Selected VRAM bank (0 or 1)
    uint8_t wramBank_;                  ///< Selected WRAM bank (1–7, 0 interpreted as 1)

    // Cartridge header information
    uint8_t cartType_;                  ///< Cartridge type (MBC)
    size_t numRomBanks_;                ///< Number of 16‑KiB ROM banks
    size_t numRamBanks_;                ///< Number of 8‑KiB RAM banks
};

} // namespace gblator

#endif // GBLATOR_MEMORY_H