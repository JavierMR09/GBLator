//
// Part of the GBLator project.
//
// This header declares a simple timer implementation for the Game Boy’s
// built‑in timer and divider registers. The timer emulates the divider
// register (DIV) at 16384 Hz and the programmable timer (TIMA) controlled
// by TAC. It increments TIMA at a rate selected by TAC bits and requests
// timer interrupts on overflow【487600738692240†L125-L171】.

#ifndef GBLATOR_TIMER_H
#define GBLATOR_TIMER_H

#include <cstdint>

namespace gblator {

class Memory;

/**
 * @brief Emulates the Game Boy’s timer and divider registers.
 *
 * The Game Boy has two timing counters: the divider register (FF04) which
 * increments at 16384 Hz, and the programmable timer (FF05–FF07) which
 * increments at a selectable frequency and triggers an interrupt on
 * overflow. This class tracks CPU cycles and updates those registers
 * accordingly【487600738692240†L125-L171】.
 */
class Timer {
public:
    explicit Timer(Memory& memory);
    /** Reset internal counters and registers. */
    void reset();
    /**
     * Step the timer by the given number of CPU cycles.
     *
     * @param cycles Number of CPU cycles elapsed
     */
    void step(int cycles);

private:
    Memory& memory_;
    int divCounter_;   ///< Counts CPU cycles until DIV increments
    int timaCounter_;  ///< Counts CPU cycles until TIMA increments

    /**
     * Compute the number of CPU cycles per TIMA increment based on TAC.
     *
     * @return Number of CPU cycles between TIMA increments (or 0 if disabled)
     */
    int timerPeriod() const;
};

} // namespace gblator

#endif // GBLATOR_TIMER_H