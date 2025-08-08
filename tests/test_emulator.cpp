//
// Unit tests for GBLator components
//
// This file implements a series of simple unit tests to verify that
// various components of the GBLator emulator behave as expected.
// Tests cover CPU instruction execution, memory bank switching, PPU
// timing and interrupt behaviour, timer operation, and joypad input
// handling. The tests use a very lightweight harness that counts
// passed and failed assertions and reports the results at the end.

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

// Define private as public to access internal state of CPU for testing
#define private public
#include "cpu/cpu.h"
#include "mmu/memory.h"
#include "utils/timer.h"
#include "ppu/ppu.h"
#include "joypad/joypad.h"
#include "apu/apu.h"
#undef private

using namespace gblator;

// Simple assertion counters
static int testsPassed = 0;
static int testsFailed = 0;

// Helper macro for assertions
#define ASSERT_EQ(actual, expected, msg)                                                     \
    do {                                                                                    \
        auto _act = (actual);                                                              \
        auto _exp = (expected);                                                            \
        if (_act != _exp) {                                                                \
            std::cerr << "FAIL: " << msg << " (expected " << int(_exp)                  \
                      << ", got " << int(_act) << ")" << std::endl;                    \
            ++testsFailed;                                                                 \
        } else {                                                                           \
            ++testsPassed;                                                                 \
        }                                                                                  \
    } while (0)

/**
 * Write a ROM image to disk. The ROM vector must contain at least
 * 0x150 bytes to hold the cartridge header. The file will be written
 * to the given path.
 */
static void writeROM(const std::string& path, const std::vector<uint8_t>& rom) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(rom.data()), static_cast<std::streamsize>(rom.size()));
    out.close();
}

// Test loading immediate values into registers using LD r,d8 instructions
static void test_ld_immediate() {
    std::cout << "Running test_ld_immediate..." << std::endl;
    Memory mem;
    // Build a small ROM with LD instructions at address 0x0100
    std::vector<uint8_t> rom(0x200, 0x00);
    size_t pc = 0x100;
    rom[pc++] = 0x06; rom[pc++] = 0x05; // LD B,0x05
    rom[pc++] = 0x0E; rom[pc++] = 0x07; // LD C,0x07
    rom[pc++] = 0x3E; rom[pc++] = 0x09; // LD A,0x09
    rom[pc++] = 0x00;                  // NOP
    // Write ROM header dummy values
    if (rom.size() > 0x150) {
        rom[0x0147] = 0x00; // ROM only
        rom[0x0148] = 0x00; // 32 KiB (2 banks)
        rom[0x0149] = 0x00; // No RAM
    }
    const std::string romPath = "test_ld_immediate.gb";
    writeROM(romPath, rom);
    bool ok = mem.loadROM(romPath);
    ASSERT_EQ(ok, true, "loadROM() succeeds");
    CPU cpu(mem);
    cpu.reset();
    // Execute LD B,0x05
    cpu.step();
    ASSERT_EQ(cpu.b_, 0x05, "LD B,d8 loads 0x05 into B");
    // Execute LD C,0x07
    cpu.step();
    ASSERT_EQ(cpu.c_, 0x07, "LD C,d8 loads 0x07 into C");
    // Execute LD A,0x09
    cpu.step();
    ASSERT_EQ(cpu.a_, 0x09, "LD A,d8 loads 0x09 into A");
}

// Test ADD A,B instruction and result stored in A
static void test_add_instruction() {
    std::cout << "Running test_add_instruction..." << std::endl;
    Memory mem;
    // ROM: LD B,0x05; LD A,0x03; ADD A,B; NOP
    std::vector<uint8_t> rom(0x200, 0x00);
    size_t pc = 0x100;
    rom[pc++] = 0x06; rom[pc++] = 0x05; // B=5
    rom[pc++] = 0x3E; rom[pc++] = 0x03; // A=3
    rom[pc++] = 0x80;                  // ADD A,B (A=8)
    rom[pc++] = 0x00;                  // NOP
    if (rom.size() > 0x150) {
        rom[0x0147] = 0x00;
        rom[0x0148] = 0x00;
        rom[0x0149] = 0x00;
    }
    const std::string romPath = "test_add.gb";
    writeROM(romPath, rom);
    mem.loadROM(romPath);
    CPU cpu(mem);
    cpu.reset();
    cpu.step(); // LD B
    cpu.step(); // LD A
    cpu.step(); // ADD A,B
    ASSERT_EQ(cpu.a_, 0x08, "ADD A,B adds B to A (3+5=8)");
}

// Test the timer: DIV increments and TIMA overflow triggers interrupt
static void test_timer() {
    std::cout << "Running test_timer..." << std::endl;
    Memory mem;
    Timer timer(mem);
    timer.reset();
    // Enable timer: TAC bit2=1 (enable), bits1-0=01 (16 cycles per increment)
    mem.writeByte(0xFF07, 0x05);
    // Set TMA = 0x00, TIMA = 0xFE to test overflow
    mem.writeByte(0xFF06, 0x00);
    mem.writeByte(0xFF05, 0xFE);
    // First increment: TIMA 0xFE -> 0xFF
    timer.step(16);
    ASSERT_EQ(mem.readByte(0xFF05), 0xFF, "TIMA increments to 0xFF");
    // Second increment: TIMA 0xFF overflows to TMA and sets IF bit 2
    timer.step(16);
    ASSERT_EQ(mem.readByte(0xFF05), 0x00, "TIMA reloads TMA after overflow");
    uint8_t iflags = mem.readByte(0xFF0F);
    ASSERT_EQ(iflags & 0x04, 0x04, "Timer interrupt requested (IF bit2)");
    // Test DIV increments every 256 cycles
    uint8_t initialDiv = mem.readByte(0xFF04);
    timer.step(256);
    uint8_t newDiv = mem.readByte(0xFF04);
    ASSERT_EQ(static_cast<uint8_t>(initialDiv + 1), newDiv, "DIV increments every 256 cycles");
}

// Test PPU scanline progression and VBlank interrupt
static void test_ppu() {
    std::cout << "Running test_ppu..." << std::endl;
    Memory mem;
    PPU ppu(mem);
    ppu.reset();
    // Enable LCD (bit 7 of LCDC)
    mem.writeByte(0xFF40, 0x80);
    // Step one scanline (114 CPU cycles) and verify LY increments
    ppu.step(114);
    ASSERT_EQ(mem.readByte(0xFF44), 1, "LY increments to 1 after one scanline");
    // Step through the rest of visible lines (143 more)
    ppu.step(114 * 143);
    ASSERT_EQ(mem.readByte(0xFF44), 144, "LY reaches 144 at start of VBlank");
    // At start of VBlank, PPU mode should be 1 and VBlank interrupt requested
    uint8_t stat = mem.readByte(0xFF41);
    ASSERT_EQ(stat & 0x03, 1, "PPU mode is VBlank (1)");
    uint8_t iflags = mem.readByte(0xFF0F);
    ASSERT_EQ(iflags & 0x01, 0x01, "VBlank interrupt requested (IF bit0)");
}

// Test ROM bank switching using MBC1
static void test_memory_bank_switch() {
    std::cout << "Running test_memory_bank_switch..." << std::endl;
    Memory mem;
    // Create a ROM with 4 banks (64 KiB). Each bank filled with a distinct value.
    const size_t bankSize = 0x4000;
    size_t romSize = 4 * bankSize;
    std::vector<uint8_t> rom(romSize, 0x00);
    // Fill banks: bank0=0x10, bank1=0x11, bank2=0x12, bank3=0x13
    for (size_t i = 0; i < 4; ++i) {
        std::fill(rom.begin() + i * bankSize, rom.begin() + (i + 1) * bankSize, static_cast<uint8_t>(0x10 + i));
    }
    // Setup header to use MBC1 and 64 KiB ROM
    if (rom.size() > 0x150) {
        rom[0x0147] = 0x01; // MBC1 without RAM
        rom[0x0148] = 0x01; // 64 KiB (4 banks)
        rom[0x0149] = 0x00; // No RAM
    }
    const std::string romPath = "test_mbc1.gb";
    writeROM(romPath, rom);
    bool ok = mem.loadROM(romPath);
    ASSERT_EQ(ok, true, "loadROM() succeeds for MBC1");
    // Default ROM bank for 4000-7FFF should be bank 1 (value 0x11)
    uint8_t val = mem.readByte(0x4000);
    ASSERT_EQ(val, 0x11, "Default ROM bank in switchable region is bank1 (0x11)");
    // Switch to bank 2 by writing 0x02 to 2000-3FFF
    mem.writeByte(0x2000, 0x02);
    val = mem.readByte(0x4000);
    ASSERT_EQ(val, 0x12, "After bank switch, reading 0x4000 yields bank2 value (0x12)");
    // Switch to bank 3
    mem.writeByte(0x2000, 0x03);
    val = mem.readByte(0x4000);
    ASSERT_EQ(val, 0x13, "After bank switch to 3, reading 0x4000 yields bank3 value (0x13)");
}

// Test joypad input handling and register behaviour
static void test_joypad() {
    std::cout << "Running test_joypad..." << std::endl;
    Memory mem;
    Joypad pad(mem);
    pad.reset();
    // After reset, JOYP register should be 0xFF (no group selected, all bits high)
    ASSERT_EQ(mem.readByte(0xFF00), 0xFF, "JOYP register initialised to 0xFF");
    // Select button group (bit4=0 selects action buttons) and press A
    mem.writeByte(0xFF00, 0x20); // bit5=1 deselect directions, bit4=0 select actions
    pad.setButton(Joypad::A, true);
    uint8_t joyp = mem.readByte(0xFF00);
    ASSERT_EQ(joyp, static_cast<uint8_t>(0xEE), "Pressing A yields JOYP=0xEE when action buttons selected");
    // Release A
    pad.setButton(Joypad::A, false);
    joyp = mem.readByte(0xFF00);
    ASSERT_EQ(joyp & 0x0F, 0x0F, "Releasing A resets lower nibble to 0xF");
    // Select direction keys (bit5=0) and press Up
    mem.writeByte(0xFF00, 0x10); // bit5=0 select directions, bit4=1 deselect actions
    pad.setButton(Joypad::Up, true);
    joyp = mem.readByte(0xFF00);
    // Expected: bits 0-3: Up (bit2) active-low -> 0, so 0x0B; bits4-5 keep original 0x10; bits6-7 = 0xC0
    ASSERT_EQ(joyp, static_cast<uint8_t>(0xDB), "Pressing Up yields JOYP=0xDB when directions selected");
}

int main() {
    test_ld_immediate();
    test_add_instruction();
    test_timer();
    test_ppu();
    test_memory_bank_switch();
    test_joypad();
    std::cout << testsPassed << " tests passed, " << testsFailed << " tests failed." << std::endl;
    return testsFailed;
}