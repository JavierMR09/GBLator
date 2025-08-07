//
// Implementation of the CPU class declared in cpu.h
//

#include "cpu/cpu.h"
#include <iostream>

namespace gblator {

CPU::CPU(Memory& memory) : memory_(memory) {
    reset();
}

void CPU::reset() {
    // Reset all registers to zero
    a_ = f_ = b_ = c_ = d_ = e_ = h_ = l_ = 0;
    // Stack pointer starts near the top of High RAM (HRAM)
    sp_ = 0xFFFE;
    // Program counter starts at 0x0100 after the boot ROM has executed【101292448676489†L117-L136】
    pc_ = 0x0100;
}

uint16_t CPU::getAF() const { return (static_cast<uint16_t>(a_) << 8) | f_; }
void CPU::setAF(uint16_t value) {
    a_ = static_cast<uint8_t>(value >> 8);
    f_ = static_cast<uint8_t>(value & 0xF0); // lower nibble of F is always zero
}
uint16_t CPU::getBC() const { return (static_cast<uint16_t>(b_) << 8) | c_; }
void CPU::setBC(uint16_t value) {
    b_ = static_cast<uint8_t>(value >> 8);
    c_ = static_cast<uint8_t>(value & 0xFF);
}
uint16_t CPU::getDE() const { return (static_cast<uint16_t>(d_) << 8) | e_; }
void CPU::setDE(uint16_t value) {
    d_ = static_cast<uint8_t>(value >> 8);
    e_ = static_cast<uint8_t>(value & 0xFF);
}
uint16_t CPU::getHL() const { return (static_cast<uint16_t>(h_) << 8) | l_; }
void CPU::setHL(uint16_t value) {
    h_ = static_cast<uint8_t>(value >> 8);
    l_ = static_cast<uint8_t>(value & 0xFF);
}

void CPU::setFlag(Flag flag, bool set) {
    if (set) {
        f_ |= flag;
    } else {
        f_ &= ~flag;
    }
}

bool CPU::getFlag(Flag flag) const {
    return (f_ & flag) != 0;
}

void CPU::step() {
    // Fetch the next opcode byte
    uint8_t opcode = memory_.readByte(pc_++);
    executeInstruction(opcode);
}

void CPU::executeInstruction(uint8_t opcode) {
    // Many Game Boy instructions follow regular patterns. Handle those
    // groupings up front using ranges, then fall back to a switch for
    // instructions that don't fit a simple pattern.

    // Define lambdas to access 8‑bit registers or memory references. The
    // register ordering used by most opcodes is: 0=B, 1=C, 2=D, 3=E,
    // 4=H, 5=L, 6=[HL], 7=A. For index 6 we read or write the byte
    // pointed to by HL.
    auto readReg8 = [&](int index) -> uint8_t {
        switch (index) {
            case 0: return b_;
            case 1: return c_;
            case 2: return d_;
            case 3: return e_;
            case 4: return h_;
            case 5: return l_;
            case 6: return memory_.readByte(getHL());
            case 7: return a_;
        }
        return 0;
    };
    auto writeReg8 = [&](int index, uint8_t value) {
        switch (index) {
            case 0: b_ = value; break;
            case 1: c_ = value; break;
            case 2: d_ = value; break;
            case 3: e_ = value; break;
            case 4: h_ = value; break;
            case 5: l_ = value; break;
            case 6: memory_.writeByte(getHL(), value); break;
            case 7: a_ = value; break;
        }
    };

    // LD r,r' family (0x40–0x7F), excluding 0x76 (HALT)
    if (opcode >= 0x40 && opcode <= 0x7F) {
        if (opcode == 0x76) {
            // HALT: halt CPU until interrupt; simplified as no-op here
            return;
        }
        int dest = (opcode >> 3) & 0x07;
        int src  = opcode & 0x07;
        uint8_t value = readReg8(src);
        writeReg8(dest, value);
        return;
    }
    // ADD A,r (0x80–0x87)
    if (opcode >= 0x80 && opcode <= 0x87) {
        int src = opcode & 0x07;
        add8(readReg8(src));
        return;
    }
    // ADC A,r (0x88–0x8F)
    if (opcode >= 0x88 && opcode <= 0x8F) {
        int src = opcode & 0x07;
        adc8(readReg8(src));
        return;
    }
    // SUB A,r (0x90–0x97)
    if (opcode >= 0x90 && opcode <= 0x97) {
        int src = opcode & 0x07;
        sub8(readReg8(src));
        return;
    }
    // SBC A,r (0x98–0x9F)
    if (opcode >= 0x98 && opcode <= 0x9F) {
        int src = opcode & 0x07;
        sbc8(readReg8(src));
        return;
    }
    // AND A,r (0xA0–0xA7)
    if (opcode >= 0xA0 && opcode <= 0xA7) {
        int src = opcode & 0x07;
        and8(readReg8(src));
        return;
    }
    // XOR A,r (0xA8–0xAF)
    if (opcode >= 0xA8 && opcode <= 0xAF) {
        int src = opcode & 0x07;
        xor8(readReg8(src));
        return;
    }
    // OR A,r (0xB0–0xB7)
    if (opcode >= 0xB0 && opcode <= 0xB7) {
        int src = opcode & 0x07;
        or8(readReg8(src));
        return;
    }
    // CP A,r (0xB8–0xBF)
    if (opcode >= 0xB8 && opcode <= 0xBF) {
        int src = opcode & 0x07;
        cp8(readReg8(src));
        return;
    }

    switch (opcode) {
        // 0x00: NOP
        case 0x00:
            // do nothing
            break;

        // 0x01: LD BC,d16
        case 0x01: {
            uint16_t value = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            setBC(value);
            break;
        }
        // 0x02: LD (BC),A
        case 0x02: {
            memory_.writeByte(getBC(), a_);
            break;
        }
        // 0x03: INC BC
        case 0x03: {
            setBC(getBC() + 1);
            break;
        }
        // 0x04: INC B
        case 0x04: {
            b_ = inc8(b_);
            break;
        }
        // 0x05: DEC B
        case 0x05: {
            b_ = dec8(b_);
            break;
        }
        // 0x06: LD B,d8
        case 0x06: {
            b_ = memory_.readByte(pc_++);
            break;
        }
        // 0x07: RLCA (rotate A left; old bit 7 -> bit 0 and carry)
        case 0x07: {
            uint8_t bit7 = (a_ & 0x80) >> 7;
            a_ = static_cast<uint8_t>((a_ << 1) | bit7);
            setFlag(Z_FLAG, false);
            setFlag(N_FLAG, false);
            setFlag(H_FLAG, false);
            setFlag(C_FLAG, bit7);
            break;
        }
        // 0x08: LD (a16),SP
        case 0x08: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            memory_.writeByte(addr, static_cast<uint8_t>(sp_ & 0xFF));
            memory_.writeByte(addr + 1, static_cast<uint8_t>(sp_ >> 8));
            break;
        }
        // 0x09: ADD HL,BC
        case 0x09: {
            addHL(getBC());
            break;
        }
        // 0x0A: LD A,(BC)
        case 0x0A: {
            a_ = memory_.readByte(getBC());
            break;
        }
        // 0x0B: DEC BC
        case 0x0B: {
            setBC(getBC() - 1);
            break;
        }
        // 0x0C: INC C
        case 0x0C: {
            c_ = inc8(c_);
            break;
        }
        // 0x0D: DEC C
        case 0x0D: {
            c_ = dec8(c_);
            break;
        }
        // 0x0E: LD C,d8
        case 0x0E: {
            c_ = memory_.readByte(pc_++);
            break;
        }
        // 0x0F: RRCA (rotate A right; old bit 0 -> bit7 and carry)
        case 0x0F: {
            uint8_t bit0 = a_ & 0x01;
            a_ = static_cast<uint8_t>((a_ >> 1) | (bit0 << 7));
            setFlag(Z_FLAG, false);
            setFlag(N_FLAG, false);
            setFlag(H_FLAG, false);
            setFlag(C_FLAG, bit0);
            break;
        }

        // 0x10: STOP (stop CPU until button pressed); treat as NOP and skip one byte
        case 0x10: {
            // The STOP instruction has a 2-byte form (0x10 0x00). We'll skip the padding byte
            pc_++;
            break;
        }
        // 0x11: LD DE,d16
        case 0x11: {
            uint16_t value = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            setDE(value);
            break;
        }
        // 0x12: LD (DE),A
        case 0x12: {
            memory_.writeByte(getDE(), a_);
            break;
        }
        // 0x13: INC DE
        case 0x13: {
            setDE(getDE() + 1);
            break;
        }
        // 0x14: INC D
        case 0x14: {
            d_ = inc8(d_);
            break;
        }
        // 0x15: DEC D
        case 0x15: {
            d_ = dec8(d_);
            break;
        }
        // 0x16: LD D,d8
        case 0x16: {
            d_ = memory_.readByte(pc_++);
            break;
        }
        // 0x17: RLA (rotate A left through carry)
        case 0x17: {
            bool oldCarry = getFlag(C_FLAG);
            uint8_t newCarry = (a_ & 0x80) >> 7;
            a_ = static_cast<uint8_t>((a_ << 1) | (oldCarry ? 1 : 0));
            setFlag(Z_FLAG, false);
            setFlag(N_FLAG, false);
            setFlag(H_FLAG, false);
            setFlag(C_FLAG, newCarry);
            break;
        }
        // 0x18: JR e8 (relative jump)
        case 0x18: {
            int8_t offset = static_cast<int8_t>(memory_.readByte(pc_++));
            pc_ = static_cast<uint16_t>(pc_ + offset);
            break;
        }
        // 0x19: ADD HL,DE
        case 0x19: {
            addHL(getDE());
            break;
        }
        // 0x1A: LD A,(DE)
        case 0x1A: {
            a_ = memory_.readByte(getDE());
            break;
        }
        // 0x1B: DEC DE
        case 0x1B: {
            setDE(getDE() - 1);
            break;
        }
        // 0x1C: INC E
        case 0x1C: {
            e_ = inc8(e_);
            break;
        }
        // 0x1D: DEC E
        case 0x1D: {
            e_ = dec8(e_);
            break;
        }
        // 0x1E: LD E,d8
        case 0x1E: {
            e_ = memory_.readByte(pc_++);
            break;
        }
        // 0x1F: RRA (rotate A right through carry)
        case 0x1F: {
            bool oldCarry = getFlag(C_FLAG);
            uint8_t newCarry = a_ & 0x01;
            a_ = static_cast<uint8_t>((a_ >> 1) | (oldCarry ? 0x80 : 0x00));
            setFlag(Z_FLAG, false);
            setFlag(N_FLAG, false);
            setFlag(H_FLAG, false);
            setFlag(C_FLAG, newCarry);
            break;
        }

        // 0x20: JR NZ,e8
        case 0x20: {
            int8_t offset = static_cast<int8_t>(memory_.readByte(pc_++));
            if (!getFlag(Z_FLAG)) {
                pc_ = static_cast<uint16_t>(pc_ + offset);
            }
            break;
        }
        // 0x21: LD HL,d16
        case 0x21: {
            uint16_t value = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            setHL(value);
            break;
        }
        // 0x22: LD (HL+),A
        case 0x22: {
            memory_.writeByte(getHL(), a_);
            setHL(getHL() + 1);
            break;
        }
        // 0x23: INC HL
        case 0x23: {
            setHL(getHL() + 1);
            break;
        }
        // 0x24: INC H
        case 0x24: {
            h_ = inc8(h_);
            break;
        }
        // 0x25: DEC H
        case 0x25: {
            h_ = dec8(h_);
            break;
        }
        // 0x26: LD H,d8
        case 0x26: {
            h_ = memory_.readByte(pc_++);
            break;
        }
        // 0x27: DAA (Decimal Adjust A)
        case 0x27: {
            uint8_t correction = 0;
            bool setC = false;
            if (!getFlag(N_FLAG)) {
                if (getFlag(H_FLAG) || (a_ & 0x0F) > 9) {
                    correction |= 0x06;
                }
                if (getFlag(C_FLAG) || a_ > 0x99) {
                    correction |= 0x60;
                    setC = true;
                }
                a_ = static_cast<uint8_t>(a_ + correction);
            } else {
                if (getFlag(H_FLAG)) {
                    correction |= 0x06;
                }
                if (getFlag(C_FLAG)) {
                    correction |= 0x60;
                    setC = true;
                }
                a_ = static_cast<uint8_t>(a_ - correction);
            }
            setFlag(H_FLAG, false);
            setFlag(Z_FLAG, a_ == 0);
            if (setC) {
                setFlag(C_FLAG, true);
            }
            // N flag remains unchanged
            break;
        }
        // 0x28: JR Z,e8
        case 0x28: {
            int8_t offset = static_cast<int8_t>(memory_.readByte(pc_++));
            if (getFlag(Z_FLAG)) {
                pc_ = static_cast<uint16_t>(pc_ + offset);
            }
            break;
        }
        // 0x29: ADD HL,HL
        case 0x29: {
            addHL(getHL());
            break;
        }
        // 0x2A: LD A,(HL+)
        case 0x2A: {
            a_ = memory_.readByte(getHL());
            setHL(getHL() + 1);
            break;
        }
        // 0x2B: DEC HL
        case 0x2B: {
            setHL(getHL() - 1);
            break;
        }
        // 0x2C: INC L
        case 0x2C: {
            l_ = inc8(l_);
            break;
        }
        // 0x2D: DEC L
        case 0x2D: {
            l_ = dec8(l_);
            break;
        }
        // 0x2E: LD L,d8
        case 0x2E: {
            l_ = memory_.readByte(pc_++);
            break;
        }
        // 0x2F: CPL (complement A)
        case 0x2F: {
            a_ = static_cast<uint8_t>(~a_);
            setFlag(N_FLAG, true);
            setFlag(H_FLAG, true);
            break;
        }

        // 0x30: JR NC,e8
        case 0x30: {
            int8_t offset = static_cast<int8_t>(memory_.readByte(pc_++));
            if (!getFlag(C_FLAG)) {
                pc_ = static_cast<uint16_t>(pc_ + offset);
            }
            break;
        }
        // 0x31: LD SP,d16
        case 0x31: {
            uint16_t value = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            sp_ = value;
            break;
        }
        // 0x32: LD (HL-),A
        case 0x32: {
            memory_.writeByte(getHL(), a_);
            setHL(getHL() - 1);
            break;
        }
        // 0x33: INC SP
        case 0x33: {
            sp_ = static_cast<uint16_t>(sp_ + 1);
            break;
        }
        // 0x34: INC (HL)
        case 0x34: {
            uint16_t addr = getHL();
            uint8_t value = memory_.readByte(addr);
            value = inc8(value);
            memory_.writeByte(addr, value);
            break;
        }
        // 0x35: DEC (HL)
        case 0x35: {
            uint16_t addr = getHL();
            uint8_t value = memory_.readByte(addr);
            value = dec8(value);
            memory_.writeByte(addr, value);
            break;
        }
        // 0x36: LD (HL),d8
        case 0x36: {
            uint8_t value = memory_.readByte(pc_++);
            memory_.writeByte(getHL(), value);
            break;
        }
        // 0x37: SCF (set carry flag)
        case 0x37: {
            setFlag(C_FLAG, true);
            setFlag(N_FLAG, false);
            setFlag(H_FLAG, false);
            break;
        }
        // 0x38: JR C,e8
        case 0x38: {
            int8_t offset = static_cast<int8_t>(memory_.readByte(pc_++));
            if (getFlag(C_FLAG)) {
                pc_ = static_cast<uint16_t>(pc_ + offset);
            }
            break;
        }
        // 0x39: ADD HL,SP
        case 0x39: {
            addHL(sp_);
            break;
        }
        // 0x3A: LD A,(HL-)
        case 0x3A: {
            a_ = memory_.readByte(getHL());
            setHL(getHL() - 1);
            break;
        }
        // 0x3B: DEC SP
        case 0x3B: {
            sp_ = static_cast<uint16_t>(sp_ - 1);
            break;
        }
        // 0x3C: INC A
        case 0x3C: {
            a_ = inc8(a_);
            break;
        }
        // 0x3D: DEC A
        case 0x3D: {
            a_ = dec8(a_);
            break;
        }
        // 0x3E: LD A,d8 (handled above but keep for completeness)
        case 0x3E: {
            a_ = memory_.readByte(pc_++);
            break;
        }
        // 0x3F: CCF (complement carry flag)
        case 0x3F: {
            bool carry = getFlag(C_FLAG);
            setFlag(C_FLAG, !carry);
            setFlag(N_FLAG, false);
            setFlag(H_FLAG, false);
            break;
        }

        // 0xC0: RET NZ
        case 0xC0: {
            if (!getFlag(Z_FLAG)) {
                uint16_t low = memory_.readByte(sp_);
                uint16_t high = memory_.readByte(sp_ + 1);
                sp_ += 2;
                pc_ = static_cast<uint16_t>((high << 8) | low);
            }
            break;
        }
        // 0xC1: POP BC
        case 0xC1: {
            uint16_t low = memory_.readByte(sp_);
            uint16_t high = memory_.readByte(sp_ + 1);
            sp_ += 2;
            uint16_t value = static_cast<uint16_t>((high << 8) | low);
            setBC(value);
            break;
        }
        // 0xC2: JP NZ,a16
        case 0xC2: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            if (!getFlag(Z_FLAG)) {
                pc_ = addr;
            }
            break;
        }
        // 0xC3: JP a16
        case 0xC3: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ = addr;
            break;
        }
        // 0xC4: CALL NZ,a16
        case 0xC4: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            if (!getFlag(Z_FLAG)) {
                // push current pc onto stack (high byte first)
                sp_ = static_cast<uint16_t>(sp_ - 1);
                memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
                sp_ = static_cast<uint16_t>(sp_ - 1);
                memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
                pc_ = addr;
            }
            break;
        }
        // 0xC5: PUSH BC
        case 0xC5: {
            uint16_t value = getBC();
            // push high then low
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((value >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(value & 0xFF));
            break;
        }
        // 0xC6: ADD A,n8
        case 0xC6: {
            uint8_t value = memory_.readByte(pc_++);
            add8(value);
            break;
        }
        // 0xC7: RST 00h
        case 0xC7: {
            // push pc then jump to 0x00
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
            pc_ = 0x00;
            break;
        }
        // 0xC8: RET Z
        case 0xC8: {
            if (getFlag(Z_FLAG)) {
                uint16_t low = memory_.readByte(sp_);
                uint16_t high = memory_.readByte(sp_ + 1);
                sp_ += 2;
                pc_ = static_cast<uint16_t>((high << 8) | low);
            }
            break;
        }
        // 0xC9: RET
        case 0xC9: {
            uint16_t low = memory_.readByte(sp_);
            uint16_t high = memory_.readByte(sp_ + 1);
            sp_ += 2;
            pc_ = static_cast<uint16_t>((high << 8) | low);
            break;
        }
        // 0xCA: JP Z,a16
        case 0xCA: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            if (getFlag(Z_FLAG)) {
                pc_ = addr;
            }
            break;
        }
        // 0xCB: PREFIX – CB-prefixed opcodes
        case 0xCB: {
            // Read the following byte and decode CB instructions. For now, print unimplemented
            uint8_t cbcode = memory_.readByte(pc_++);
            std::cerr << "Unimplemented CB opcode: 0x" << std::hex
                      << static_cast<int>(cbcode) << std::dec << "\n";
            break;
        }
        // 0xCC: CALL Z,a16
        case 0xCC: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            if (getFlag(Z_FLAG)) {
                sp_ = static_cast<uint16_t>(sp_ - 1);
                memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
                sp_ = static_cast<uint16_t>(sp_ - 1);
                memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
                pc_ = addr;
            }
            break;
        }
        // 0xCD: CALL a16
        case 0xCD: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            // push current pc
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
            pc_ = addr;
            break;
        }
        // 0xCE: ADC A,n8
        case 0xCE: {
            uint8_t value = memory_.readByte(pc_++);
            adc8(value);
            break;
        }
        // 0xCF: RST 08h
        case 0xCF: {
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
            pc_ = 0x08;
            break;
        }
        // 0xD0: RET NC
        case 0xD0: {
            if (!getFlag(C_FLAG)) {
                uint16_t low = memory_.readByte(sp_);
                uint16_t high = memory_.readByte(sp_ + 1);
                sp_ += 2;
                pc_ = static_cast<uint16_t>((high << 8) | low);
            }
            break;
        }
        // 0xD1: POP DE
        case 0xD1: {
            uint16_t low = memory_.readByte(sp_);
            uint16_t high = memory_.readByte(sp_ + 1);
            sp_ += 2;
            uint16_t value = static_cast<uint16_t>((high << 8) | low);
            setDE(value);
            break;
        }
        // 0xD2: JP NC,a16
        case 0xD2: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            if (!getFlag(C_FLAG)) {
                pc_ = addr;
            }
            break;
        }
        // 0xD4: CALL NC,a16
        case 0xD4: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            if (!getFlag(C_FLAG)) {
                sp_ = static_cast<uint16_t>(sp_ - 1);
                memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
                sp_ = static_cast<uint16_t>(sp_ - 1);
                memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
                pc_ = addr;
            }
            break;
        }
        // 0xD5: PUSH DE
        case 0xD5: {
            uint16_t value = getDE();
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((value >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(value & 0xFF));
            break;
        }
        // 0xD6: SUB n8
        case 0xD6: {
            uint8_t value = memory_.readByte(pc_++);
            sub8(value);
            break;
        }
        // 0xD7: RST 10h
        case 0xD7: {
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
            pc_ = 0x10;
            break;
        }
        // 0xD8: RET C
        case 0xD8: {
            if (getFlag(C_FLAG)) {
                uint16_t low = memory_.readByte(sp_);
                uint16_t high = memory_.readByte(sp_ + 1);
                sp_ += 2;
                pc_ = static_cast<uint16_t>((high << 8) | low);
            }
            break;
        }
        // 0xD9: RETI (return and enable interrupts)
        case 0xD9: {
            uint16_t low = memory_.readByte(sp_);
            uint16_t high = memory_.readByte(sp_ + 1);
            sp_ += 2;
            pc_ = static_cast<uint16_t>((high << 8) | low);
            // enabling interrupts would be handled here; ignore for now
            break;
        }
        // 0xDA: JP C,a16
        case 0xDA: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            if (getFlag(C_FLAG)) {
                pc_ = addr;
            }
            break;
        }
        // 0xDC: CALL C,a16
        case 0xDC: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            if (getFlag(C_FLAG)) {
                sp_ = static_cast<uint16_t>(sp_ - 1);
                memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
                sp_ = static_cast<uint16_t>(sp_ - 1);
                memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
                pc_ = addr;
            }
            break;
        }
        // 0xDE: SBC A,n8
        case 0xDE: {
            uint8_t value = memory_.readByte(pc_++);
            sbc8(value);
            break;
        }
        // 0xDF: RST 18h
        case 0xDF: {
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
            pc_ = 0x18;
            break;
        }

        // 0xE0: LDH (n8),A
        case 0xE0: {
            uint8_t offset = memory_.readByte(pc_++);
            memory_.writeByte(static_cast<uint16_t>(0xFF00u + offset), a_);
            break;
        }
        // 0xE1: POP HL
        case 0xE1: {
            uint16_t low = memory_.readByte(sp_);
            uint16_t high = memory_.readByte(sp_ + 1);
            sp_ += 2;
            uint16_t value = static_cast<uint16_t>((high << 8) | low);
            setHL(value);
            break;
        }
        // 0xE2: LD (C),A (aka LDH [C],A)
        case 0xE2: {
            memory_.writeByte(static_cast<uint16_t>(0xFF00u + c_), a_);
            break;
        }
        // 0xE5: PUSH HL
        case 0xE5: {
            uint16_t value = getHL();
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((value >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(value & 0xFF));
            break;
        }
        // 0xE6: AND n8
        case 0xE6: {
            uint8_t value = memory_.readByte(pc_++);
            and8(value);
            break;
        }
        // 0xE7: RST 20h
        case 0xE7: {
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
            pc_ = 0x20;
            break;
        }
        // 0xE8: ADD SP,e8
        case 0xE8: {
            int8_t e8 = static_cast<int8_t>(memory_.readByte(pc_++));
            uint16_t spOld = sp_;
            uint16_t result = static_cast<uint16_t>(spOld + e8);
            setFlag(Z_FLAG, false);
            setFlag(N_FLAG, false);
            // half-carry and carry from lower 8 bits
            setFlag(H_FLAG, ((spOld & 0x0F) + (e8 & 0x0F)) > 0x0F);
            setFlag(C_FLAG, ((spOld & 0xFF) + (static_cast<uint16_t>(e8) & 0xFF)) > 0xFF);
            sp_ = result;
            break;
        }
        // 0xE9: JP (HL)
        case 0xE9: {
            pc_ = getHL();
            break;
        }
        // 0xEA: LD (a16),A
        case 0xEA: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            memory_.writeByte(addr, a_);
            break;
        }
        // 0xEE: XOR n8
        case 0xEE: {
            uint8_t value = memory_.readByte(pc_++);
            xor8(value);
            break;
        }
        // 0xEF: RST 28h
        case 0xEF: {
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
            pc_ = 0x28;
            break;
        }

        // 0xF0: LDH A,(n8)
        case 0xF0: {
            uint8_t offset = memory_.readByte(pc_++);
            a_ = memory_.readByte(static_cast<uint16_t>(0xFF00u + offset));
            break;
        }
        // 0xF1: POP AF
        case 0xF1: {
            uint16_t low = memory_.readByte(sp_);
            uint16_t high = memory_.readByte(sp_ + 1);
            sp_ += 2;
            uint16_t value = static_cast<uint16_t>((high << 8) | low);
            setAF(value);
            break;
        }
        // 0xF2: LD A,(C)
        case 0xF2: {
            a_ = memory_.readByte(static_cast<uint16_t>(0xFF00u + c_));
            break;
        }
        // 0xF3: DI (disable interrupts)
        case 0xF3: {
            // Interrupt disable would clear IME; not implemented here
            break;
        }
        // 0xF5: PUSH AF
        case 0xF5: {
            uint16_t value = getAF();
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((value >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(value & 0xFF));
            break;
        }
        // 0xF6: OR n8
        case 0xF6: {
            uint8_t value = memory_.readByte(pc_++);
            or8(value);
            break;
        }
        // 0xF7: RST 30h
        case 0xF7: {
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
            pc_ = 0x30;
            break;
        }
        // 0xF8: LD HL,SP+e8
        case 0xF8: {
            int8_t e8 = static_cast<int8_t>(memory_.readByte(pc_++));
            uint16_t spOld = sp_;
            uint16_t result = static_cast<uint16_t>(spOld + e8);
            setFlag(Z_FLAG, false);
            setFlag(N_FLAG, false);
            setFlag(H_FLAG, ((spOld & 0x0F) + (e8 & 0x0F)) > 0x0F);
            setFlag(C_FLAG, ((spOld & 0xFF) + (static_cast<uint16_t>(e8) & 0xFF)) > 0xFF);
            setHL(result);
            break;
        }
        // 0xF9: LD SP,HL
        case 0xF9: {
            sp_ = getHL();
            break;
        }
        // 0xFA: LD A,(a16)
        case 0xFA: {
            uint16_t addr = memory_.readByte(pc_) | (memory_.readByte(pc_ + 1) << 8);
            pc_ += 2;
            a_ = memory_.readByte(addr);
            break;
        }
        // 0xFB: EI (enable interrupts)
        case 0xFB: {
            // Would set IME=1 after next instruction; not implemented
            break;
        }
        // 0xFE: CP n8
        case 0xFE: {
            uint8_t value = memory_.readByte(pc_++);
            cp8(value);
            break;
        }
        // 0xFF: RST 38h
        case 0xFF: {
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>((pc_ >> 8) & 0xFF));
            sp_ = static_cast<uint16_t>(sp_ - 1);
            memory_.writeByte(sp_, static_cast<uint8_t>(pc_ & 0xFF));
            pc_ = 0x38;
            break;
        }

        default: {
            std::cerr << "Unimplemented opcode: 0x" << std::hex
                      << static_cast<int>(opcode) << std::dec << "\n";
            break;
        }
    }
}

// ----- Helper functions -----

/**
 * Increment an 8-bit value and update Z, N and H flags. Carry flag is preserved.
 */
uint8_t CPU::inc8(uint8_t value) {
    uint8_t result = static_cast<uint8_t>(value + 1);
    // Preserve carry
    bool carry = getFlag(C_FLAG);
    setFlag(Z_FLAG, result == 0);
    setFlag(N_FLAG, false);
    setFlag(H_FLAG, ((value & 0x0F) + 1) > 0x0F);
    setFlag(C_FLAG, carry);
    return result;
}

/**
 * Decrement an 8-bit value and update Z, N and H flags. Carry flag is preserved.
 */
uint8_t CPU::dec8(uint8_t value) {
    uint8_t result = static_cast<uint8_t>(value - 1);
    bool carry = getFlag(C_FLAG);
    setFlag(Z_FLAG, result == 0);
    setFlag(N_FLAG, true);
    // Half borrow when lower nibble becomes negative
    setFlag(H_FLAG, (value & 0x0F) == 0);
    setFlag(C_FLAG, carry);
    return result;
}

/**
 * Add an 8-bit value to A and update flags.
 */
void CPU::add8(uint8_t value) {
    uint16_t result = static_cast<uint16_t>(a_) + value;
    setFlag(Z_FLAG, static_cast<uint8_t>(result) == 0);
    setFlag(N_FLAG, false);
    setFlag(H_FLAG, ((a_ & 0x0F) + (value & 0x0F)) > 0x0F);
    setFlag(C_FLAG, result > 0xFF);
    a_ = static_cast<uint8_t>(result);
}

/**
 * Add an 8-bit value and carry flag to A and update flags.
 */
void CPU::adc8(uint8_t value) {
    uint16_t carry = getFlag(C_FLAG) ? 1 : 0;
    uint16_t result = static_cast<uint16_t>(a_) + value + carry;
    setFlag(Z_FLAG, static_cast<uint8_t>(result) == 0);
    setFlag(N_FLAG, false);
    setFlag(H_FLAG, ((a_ & 0x0F) + (value & 0x0F) + carry) > 0x0F);
    setFlag(C_FLAG, result > 0xFF);
    a_ = static_cast<uint8_t>(result);
}

/**
 * Subtract an 8-bit value from A and update flags.
 */
void CPU::sub8(uint8_t value) {
    uint16_t result = static_cast<uint16_t>(a_) - value;
    setFlag(Z_FLAG, static_cast<uint8_t>(result) == 0);
    setFlag(N_FLAG, true);
    setFlag(H_FLAG, (a_ & 0x0F) < (value & 0x0F));
    setFlag(C_FLAG, a_ < value);
    a_ = static_cast<uint8_t>(result);
}

/**
 * Subtract an 8-bit value and carry flag from A and update flags.
 */
void CPU::sbc8(uint8_t value) {
    uint16_t carry = getFlag(C_FLAG) ? 1 : 0;
    uint16_t result = static_cast<uint16_t>(a_) - value - carry;
    setFlag(Z_FLAG, static_cast<uint8_t>(result) == 0);
    setFlag(N_FLAG, true);
    setFlag(H_FLAG, (a_ & 0x0F) < ((value & 0x0F) + carry));
    setFlag(C_FLAG, a_ < (value + carry));
    a_ = static_cast<uint8_t>(result);
}

/**
 * Logical AND between A and value; updates flags.
 */
void CPU::and8(uint8_t value) {
    a_ &= value;
    setFlag(Z_FLAG, a_ == 0);
    setFlag(N_FLAG, false);
    setFlag(H_FLAG, true);
    setFlag(C_FLAG, false);
}

/**
 * Logical OR between A and value; updates flags.
 */
void CPU::or8(uint8_t value) {
    a_ |= value;
    setFlag(Z_FLAG, a_ == 0);
    setFlag(N_FLAG, false);
    setFlag(H_FLAG, false);
    setFlag(C_FLAG, false);
}

/**
 * Logical XOR between A and value; updates flags.
 */
void CPU::xor8(uint8_t value) {
    a_ ^= value;
    setFlag(Z_FLAG, a_ == 0);
    setFlag(N_FLAG, false);
    setFlag(H_FLAG, false);
    setFlag(C_FLAG, false);
}

/**
 * Compare value with A (CP instruction).
 */
void CPU::cp8(uint8_t value) {
    uint16_t result = static_cast<uint16_t>(a_) - value;
    setFlag(Z_FLAG, static_cast<uint8_t>(result) == 0);
    setFlag(N_FLAG, true);
    setFlag(H_FLAG, (a_ & 0x0F) < (value & 0x0F));
    setFlag(C_FLAG, a_ < value);
}

/**
 * Add a 16-bit value to HL; updates N=0, H and C flags.
 */
void CPU::addHL(uint16_t value) {
    uint32_t hl = getHL();
    uint32_t result = hl + value;
    setFlag(N_FLAG, false);
    setFlag(H_FLAG, ((hl & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF);
    setFlag(C_FLAG, result > 0xFFFF);
    setHL(static_cast<uint16_t>(result));
}

} // namespace gblator