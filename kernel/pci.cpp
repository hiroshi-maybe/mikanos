#include "pci.hpp"
#include "asmfunc.h"

namespace {
  using namespace pci;

    // Create the value for CONFIG_ADDRESS register
    uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t func,
        uint8_t reg_addr)
    {
        auto shl = [](uint32_t x, unsigned int bits) {
            return x << bits;
        };

        return shl(1, 32)
            | shl(bus, 16)
            | shl(device, 11)
            | shl(func, 8)
            | (reg_addr & 0xfcu);
    }
}

namespace pci {
    // Write to the CONFIG_ADDRESS register in the IO address space
    void WriteAddress(uint32_t address) {
        IoOut32(kConfigAddress, address);
    }

    // Write a value to the CONFIG_DATA register in the IO address space
    void WriteData(uint32_t value) {
        IoOut32(kConfigData, value);
    }

    // Read the data in the CONFIG_DATA register
    uint32_t ReadData() {
        return IoIn32(kConfigData);
    }

    // Read a vendor ID loaded in the CONFIG_DATA register
    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t func) {
        WriteAddress(MakeAddress(bus, device, func, 0x00));
        return ReadData() & 0xffffu;
    }
}