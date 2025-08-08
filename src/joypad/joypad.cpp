//
// Implementation of the Joypad class.
//

#include "joypad/joypad.h"
#include "mmu/memory.h"

namespace gblator {

Joypad::Joypad(Memory& memory) : memory_(memory), buttonState_(0x00) {
}

void Joypad::reset() {
    buttonState_ = 0x00;
    // Initialize JOYP register to no buttons selected, all buttons unpressed
    memory_.writeByte(0xFF00, 0xFF);
}

void Joypad::setButton(Button button, bool pressed) {
    uint8_t mask = static_cast<uint8_t>(1 << static_cast<uint8_t>(button));
    if (pressed) {
        buttonState_ |= mask;
    } else {
        buttonState_ &= static_cast<uint8_t>(~mask);
    }
    updateRegister();
}

void Joypad::updateRegister() {
    uint8_t joyp = memory_.readByte(0xFF00);
    // Bits 4 and 5 select button groups: 0 = select, 1 = deselect
    bool selectButtons = (joyp & 0x10) == 0;
    bool selectDirections = (joyp & 0x20) == 0;
    uint8_t lowNibble = 0x0F;
    if (selectButtons) {
        // Bits 3..0 map to Start, Select, B, A
        // Buttons are active-low
        lowNibble &= static_cast<uint8_t>(~((buttonState_ >> 4) & 0x0F));
    }
    if (selectDirections) {
        // Bits 3..0 map to Down, Up, Left, Right
        lowNibble &= static_cast<uint8_t>(~(buttonState_ & 0x0F));
    }
    // Bits 4 and 5 remain as written by CPU
    joyp = static_cast<uint8_t>((joyp & 0x30) | (lowNibble & 0x0F) | 0xC0);
    memory_.writeByte(0xFF00, joyp);
}

} // namespace gblator