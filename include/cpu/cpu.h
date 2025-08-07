//
// Copyright (c) 2025.
//
// This file is part of the GBLator project. It declares the CPU class
// which emulates the Sharp LR35902 processor used in the Nintendo Game
// Boy. The CPU has eight 8‑bit registers that can be paired to form
// 16‑bit registers (AF, BC, DE, HL), and two special 16‑bit registers:
// the program counter (PC) and the stack pointer (SP). For a quick
// overview of these registers and the meaning of the flags see the
// Pan Docs section on CPU registers and flags【847291178566708†L117-L139】.

#ifndef GBLATOR_CPU_H
#define GBLATOR_CPU_H

#include <cstdint>
#include "mmu/memory.h"

namespace gblator {

/**
 * @brief Simple emulation of the Game Boy CPU.
 *
 * The Sharp LR35902 CPU is an 8‑bit CPU with many similarities to the
 * Intel 8080 and Z80. This class models the CPU state (registers and
 * flags) and provides methods to reset the CPU and execute one
 * instruction at a time. Instruction decoding and execution is very
 * incomplete at this early stage; it will be extended as the emulator
 * develops.
 */
class CPU {
public:
    /**
     * @brief Construct a new CPU object attached to a Memory instance.
     *
     * @param memory Reference to the emulator's memory subsystem. The CPU
     * reads instructions and data from this memory and writes results back
     * to it.
     */
    explicit CPU(Memory& memory);

    /**
     * @brief Reset the CPU to its initial state.
     *
     * Sets registers to zero and initializes the stack pointer and
     * program counter. According to the Game Boy hardware, the boot ROM
     * typically leaves SP at 0xFFFE and jumps to 0x0100 to start
     * execution【101292448676489†L117-L136】. We adopt those values here.
     */
    void reset();

    /**
     * @brief Execute a single instruction at the current program counter.
     *
     * Fetches the opcode byte at the current PC, increments the PC, and
     * dispatches execution. At present only a very small subset of
     * opcodes is implemented; unimplemented opcodes will print a message
     * to standard error.
     */
    void step();

private:
    // 8‑bit registers
    uint8_t a_{0}, f_{0}, b_{0}, c_{0}, d_{0}, e_{0}, h_{0}, l_{0};
    // Stack pointer and program counter
    uint16_t sp_{0}, pc_{0};
    // Reference to memory
    Memory& memory_;

    // Helpers to get and set combined 16‑bit registers
    uint16_t getAF() const;
    void setAF(uint16_t value);
    uint16_t getBC() const;
    void setBC(uint16_t value);
    uint16_t getDE() const;
    void setDE(uint16_t value);
    uint16_t getHL() const;
    void setHL(uint16_t value);

    /// CPU flags as bit masks within the F register
    enum Flag : uint8_t {
        Z_FLAG = 1u << 7, //!< Zero flag【847291178566708†L143-L146】
        N_FLAG = 1u << 6, //!< Subtract flag (BCD)【847291178566708†L143-L160】
        H_FLAG = 1u << 5, //!< Half carry flag (BCD)【847291178566708†L133-L139】
        C_FLAG = 1u << 4  //!< Carry flag【847291178566708†L148-L155】
    };

    /**
     * @brief Set or clear a CPU flag.
     *
     * @param flag Flag mask to modify
     * @param set True to set the flag, false to clear it
     */
    void setFlag(Flag flag, bool set);

    /**
     * @brief Test whether a CPU flag is set.
     *
     * @param flag Flag mask to test
     * @return true if the flag is currently set
     */
    bool getFlag(Flag flag) const;

    /**
     * @brief Dispatch execution of a single opcode.
     *
     * @param opcode The instruction opcode to execute
     */
    void executeInstruction(uint8_t opcode);

    // Helper arithmetic and flag update methods
    /**
     * @brief Increment an 8-bit value and update CPU flags.
     *
     * This helper updates Z, N and H flags according to the Game Boy
     * specification for INC r instructions. The carry flag is preserved.
     *
     * @param value The original 8‑bit value
     * @return The incremented value
     */
    uint8_t inc8(uint8_t value);

    /**
     * @brief Decrement an 8-bit value and update CPU flags.
     *
     * This helper updates Z, N and H flags according to the Game Boy
     * specification for DEC r instructions. The carry flag is preserved.
     *
     * @param value The original 8‑bit value
     * @return The decremented value
     */
    uint8_t dec8(uint8_t value);

    /**
     * @brief Add a value to register A and update flags.
     *
     * Implements the ADD A,n instruction. Sets Z, N=0, H and C flags.
     *
     * @param value The value to add to A
     */
    void add8(uint8_t value);

    /**
     * @brief Add a value plus carry to register A and update flags.
     *
     * Implements the ADC A,n instruction. Includes the current carry flag
     * in the addition.
     *
     * @param value The value to add to A
     */
    void adc8(uint8_t value);

    /**
     * @brief Subtract a value from register A and update flags.
     *
     * Implements the SUB n instruction. Sets Z, N=1, H and C flags.
     *
     * @param value The value to subtract from A
     */
    void sub8(uint8_t value);

    /**
     * @brief Subtract a value plus carry from register A and update flags.
     *
     * Implements the SBC A,n instruction. Includes the current carry flag
     * in the subtraction.
     *
     * @param value The value to subtract from A
     */
    void sbc8(uint8_t value);

    /**
     * @brief Logical AND with register A.
     *
     * Sets register A to (A & value) and updates flags accordingly
     * (Z flag set if result is zero, N=0, H=1, C=0).
     *
     * @param value The value to AND with A
     */
    void and8(uint8_t value);

    /**
     * @brief Logical OR with register A.
     *
     * Sets register A to (A | value) and updates flags accordingly
     * (Z flag set if result is zero, N=0, H=0, C=0).
     *
     * @param value The value to OR with A
     */
    void or8(uint8_t value);

    /**
     * @brief Logical XOR with register A.
     *
     * Sets register A to (A ^ value) and updates flags accordingly
     * (Z flag set if result is zero, N=0, H=0, C=0).
     *
     * @param value The value to XOR with A
     */
    void xor8(uint8_t value);

    /**
     * @brief Compare a value with register A (CP instruction).
     *
     * Performs A - value, updates flags (Z,N,H,C) but does not store
     * the result back into A.
     *
     * @param value The value to compare against A
     */
    void cp8(uint8_t value);

    /**
     * @brief Add a 16-bit value to HL and update flags.
     *
     * Implements ADD HL,rr instructions. Sets N=0, and updates H and C
     * according to bit 11 and bit 15 carries. Does not affect Z.
     *
     * @param value The 16-bit value to add to HL
     */
    void addHL(uint16_t value);
};

} // namespace gblator

#endif // GBLATOR_CPU_H