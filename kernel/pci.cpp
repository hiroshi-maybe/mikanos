#include "pci.hpp"
#include "asmfunc.h"

const uint8_t LAST_8BIT = 0xffu;

namespace {
    using namespace pci;
    const uint16_t INVALID_VENDOR_ID = 0xffffu;

    // Create the value for CONFIG_ADDRESS register
    uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t func,
        uint8_t reg_addr)
    {
        auto shl = [](uint32_t x, unsigned int bits) {
            return x << bits;
        };

        return shl(1, 31)
            | shl(bus, 16)
            | shl(device, 11)
            | shl(func, 8)
            | (reg_addr & 0xfcu); // 0xfcu = 11111100(2)
    }

    Error AddDevice(const Device& device) {
        if (num_device == devices.size()) {
            return MAKE_ERROR(Error::kFull);
        }

        devices[num_device++] = device;
        return MAKE_ERROR(Error::kSuccess);
    }

    Error ScanBus(uint8_t bus);

    Error ScanFunction(uint8_t bus, uint8_t device, uint8_t func) {
        auto class_code = ReadClassCode(bus, device, func);
        auto header_type = ReadHeaderType(bus, device, func);
        Device dev {bus, device, func, header_type, class_code};
        if (auto err = AddDevice(dev)) return err;

        if (class_code.base == 0x06u && class_code.sub == 0x04u) {
            // standard PCI-PCI bridge
            auto bus_numbers = ReadBusNumbers(bus, device, func);
            uint8_t secondary_bus = (bus_numbers >> 8) & LAST_8BIT;
            return ScanBus(secondary_bus);
        }

        return MAKE_ERROR(Error::kSuccess);
    }

    Error ScanDevice(uint8_t bus, uint8_t device) {
        if (auto err = ScanFunction(bus, device, 0)) {
            return err;
        }

        if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
            return MAKE_ERROR(Error::kSuccess);
        }

        for (uint8_t func = 1; func < 8; ++func) {
            if(ReadVendorId(bus, device, func) == INVALID_VENDOR_ID) {
                continue;
            }

            if(auto err = ScanFunction(bus, device, func)) {
                return err;
            }
        }

        return MAKE_ERROR(Error::kSuccess);
    }

    Error ScanBus(uint8_t bus) {
        for (uint8_t device = 0; device < 32; ++device) {
            if(ReadVendorId(bus, device, 0) == INVALID_VENDOR_ID) {
                continue;
            }

            if(auto err = ScanDevice(bus, device)) {
                return err;
            }
        }

        return MAKE_ERROR(Error::kSuccess);
    }

    uint32_t ReadPCIConfig(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
        WriteAddress(MakeAddress(bus, device, func, offset));
        return ReadData();
    }
}

namespace pci {
    void WriteAddress(uint32_t address) {
        IoOut32(kConfigAddress, address);
    }

    void WriteData(uint32_t value) {
        IoOut32(kConfigData, value);
    }

    uint32_t ReadData() {
        return IoIn32(kConfigData);
    }

    bool IsSingleFunctionDevice(uint8_t header_type) {
        // 7-th bit is ON iff the device is multi-function
        return (header_type & 0x80u) == 0;
    }

    uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t func) {
        uint32_t config = ReadPCIConfig(bus, device, func, 0x00);
        return config >> 16; // first 16 bit
    }

    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t func) {
        uint32_t config = ReadPCIConfig(bus, device, func, 0x00);
        return config & 0xffffu; // last 16 bit
    }

    ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t func) {
        uint32_t config = ReadPCIConfig(bus, device, func, 0x08);
        ClassCode cc;
        cc.base = (config >> 24) & LAST_8BIT;
        cc.sub = (config >> 16) & LAST_8BIT;
        cc.interface = (config >> 8) & LAST_8BIT;
        return cc;
    }

    uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t func) {
        uint32_t config = ReadPCIConfig(bus, device, func, 0x0c);
        return (config >> 16) & LAST_8BIT;
    }

    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t func) {
        return ReadPCIConfig(bus, device, func, 0x18);
    }

    Error ScanAllBus() {
        num_device = 0;

        auto header_type = ReadHeaderType(0, 0, 0);
        if (IsSingleFunctionDevice(header_type)) {
            return ScanBus(0);
        }

        for (uint8_t func = 1; func < 8; ++func) {
            if (ReadVendorId(0, 0, func) == INVALID_VENDOR_ID) {
                continue;
            }

            if (auto err = ScanBus(func)) {
                return err;
            }
        }

        return MAKE_ERROR(Error::kSuccess);
    }
}
