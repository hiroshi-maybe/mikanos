#pragma once

#include <array>
#include <cstdint>

#include "error.hpp"

namespace pci {
    // Address of CONFIG_ADDRESS register in IO address space
    const uint16_t kConfigAddress = 0x0cf8;
    // Address of CONFIG_DATA register in IO address space
    const uint16_t kConfigData = 0x0cfc;

    Error ScanAllBus();

    struct ClassCode {
        uint8_t base, sub, interface;
    };

    struct Device {
        uint8_t bus, device, function, header_type;
        ClassCode class_code;
    };

    // An array for the list of found devices
    inline std::array<Device, 32> devices;
    // The number of found devices
    inline int num_device;

    // Write to the CONFIG_ADDRESS register in the IO address space
    void WriteAddress(uint32_t address);

    // Write a value to the CONFIG_DATA register in the IO address space
    void WriteData(uint32_t value);

    // Read the data in the CONFIG_DATA register
    uint32_t ReadData();

    uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
    uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
    ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);
    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

    bool IsSingleFunctionDevice(uint8_t header_type);
};