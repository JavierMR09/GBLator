//
// Part of the GBLator project.
//
// This header declares a simple Joypad class that manages Game Boy
// button states and exposes them via the P1/JOYP register at FF00. The
// eight buttons are arranged in a 2×4 matrix (direction and action) and
// reading the register returns the selected group of buttons (active-low)
//【58049040283099†L119-L137】.

#ifndef GBLATOR_JOYPAD_H
#define GBLATOR_JOYPAD_H

#include <cstdint>

namespace gblator {

class Memory;

/**
 * @brief Represents the Game Boy joypad input.
 *
 * Buttons are active-low: a pressed button reads as 0. The P1/JOYP
 * register selects between direction keys and action buttons and
 * returns the state of the selected group【58049040283099†L119-L137】.
 */
class Joypad {
public:
    enum Button {
        Right = 0,
        Left = 1,
        Up = 2,
        Down = 3,
        A = 4,
        B = 5,
        Select = 6,
        Start = 7
    };

    explicit Joypad(Memory& memory);
    /** Reset button states and JOYP register. */
    void reset();
    /** Press or release a button. */
    void setButton(Button button, bool pressed);
    /** Update the JOYP register to reflect current button states. */
    void updateRegister();

private:
    Memory& memory_;
    uint8_t buttonState_; ///< Bitmask of button states (1=pressed, 0=released)
};

} // namespace gblator

#endif // GBLATOR_JOYPAD_H