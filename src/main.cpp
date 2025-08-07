//
// Entry point for the GBLator emulator.
//

#include "mmu/memory.h"
#include "cpu/cpu.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ROM file>\n";
        return 1;
    }
    const char* romPath = argv[1];

    // Create memory and load the ROM into it
    gblator::Memory memory;
    if (!memory.loadROM(romPath)) {
        std::cerr << "Failed to load ROM file: " << romPath << "\n";
        return 1;
    }

    // Create CPU attached to memory and reset it
    gblator::CPU cpu(memory);
    cpu.reset();

    // Execute a limited number of instructions to demonstrate the setup
    // In a full emulator this loop would continue until the program ends
    for (int i = 0; i < 50; ++i) {
        cpu.step();
    }

    return 0;
}