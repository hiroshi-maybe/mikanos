#pragma once

#include <cstdint>

namespace pci {
    // Address of CONFIG_ADDRESS register in IO address space
    const uint16_t kConfigAddress = 0x0cf8;
    // Address of CONFIG_DATA register in IO address space
    const uint16_t kConfigData = 0x0cfc;
}