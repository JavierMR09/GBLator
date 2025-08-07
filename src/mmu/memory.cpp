//
// Implementation of the Memory class defined in memory.h
//

#include "mmu/memory.h"
#include <algorithm>
#include <fstream>
#include <iterator>

namespace gblator {

Memory::Memory() : data_(kAddressSpaceSize, 0) {
}

uint8_t Memory::readByte(uint16_t address) const {
    // Simple bounds check; if address is beyond the size of the
    // address space we wrap around. This mirrors the behaviour of
    // the Game Boy’s 16‑bit address bus.
    return data_[address % kAddressSpaceSize];
}

void Memory::writeByte(uint16_t address, uint8_t value) {
    data_[address % kAddressSpaceSize] = value;
}

bool Memory::loadROM(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    // Read file contents into a temporary buffer
    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // Copy as much of the ROM as will fit into memory beginning at address 0
    size_t loadSize = std::min(rom.size(), data_.size());
    std::copy(rom.begin(), rom.begin() + loadSize, data_.begin());
    return true;
}

void Memory::reset() {
    std::fill(data_.begin(), data_.end(), 0);
}

} // namespace gblator