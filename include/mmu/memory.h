//
// Copyright (c) 2025.
//
// This file is part of the GBLator project, a simple Nintendo Game Boy
// emulator written in C++ as a learning exercise. The code in this file
// provides a very basic memory subsystem that emulates the Game Boy’s
// 16‑bit address space. It includes methods to read and write bytes and
// load a ROM into memory. For more details on the Game Boy memory map
// (ROM banks, VRAM, work RAM and I/O registers) refer to the Pan Docs
// memory map section【101292448676489†L117-L136】.

#ifndef GBLATOR_MEMORY_H
#define GBLATOR_MEMORY_H

#include <cstdint>
#include <vector>
#include <string>

namespace gblator {

/**
 * @brief Represents the Game Boy's 64‑KiB address space.
 *
 * The Game Boy uses a 16‑bit address bus to address ROM, RAM and I/O
 * registers【101292448676489†L117-L136】. This class models that address space
 * as a flat array of bytes. Real Game Boy hardware includes a lot of
 * complicated memory mapping logic (for example MBCs for bank switching),
 * but this simple implementation treats the address space as a
 * contiguous array. This is sufficient to get a very early emulator
 * running and can later be extended with proper memory bank controllers.
 */
class Memory {
public:
    /// Size of the Game Boy address space in bytes (64 KiB)
    static const size_t kAddressSpaceSize = 0x10000;

    /**
     * @brief Construct a new Memory object.
     *
     * Initializes all bytes in the address space to zero.
     */
    Memory();

    /**
     * @brief Read a byte from the given address.
     *
     * @param address 16‑bit address to read from
     * @return The byte stored at the specified address
     */
    uint8_t readByte(uint16_t address) const;

    /**
     * @brief Write a byte to the given address.
     *
     * @param address 16‑bit address to write to
     * @param value The byte to store at the specified address
     */
    void writeByte(uint16_t address, uint8_t value);

    /**
     * @brief Load a ROM file into the beginning of memory.
     *
     * The Game Boy expects the first ROM bank to be mapped at
     * 0x0000–0x3FFF【101292448676489†L117-L136】. This method reads the contents of
     * the given file and copies as many bytes as will fit into memory,
     * starting at address 0.
     *
     * @param filepath Path to the ROM file on disk
     * @return true on success, false if the file could not be opened
     */
    bool loadROM(const std::string& filepath);

    /**
     * @brief Reset all memory to zero.
     */
    void reset();

private:
    std::vector<uint8_t> data_;
};

} // namespace gblator

#endif // GBLATOR_MEMORY_H