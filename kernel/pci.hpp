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
        bool Match(uint8_t b) { return b == base; }
        bool Match(uint8_t b, uint8_t s) { return Match(b) && s == sub; }
        bool Match(uint8_t b, uint8_t s, uint8_t i) { return Match(b, s) && i == interface; }
        bool Match(ClassCode other) {
            return Match(other.base, other.sub, other.interface);
        }

        const static uint8_t BASE_SERIAL_BUS_CONTROLLER = 0x0cu;
        const static uint8_t SUB_USB_CONTROLLER = 0x03u;
        const static uint8_t INTERFACE_XHCI = 0x30u;
    };

    const ClassCode CLASSCODE_EHCI = ClassCode{
        ClassCode::BASE_SERIAL_BUS_CONTROLLER,
        ClassCode::SUB_USB_CONTROLLER,
        ClassCode::INTERFACE_XHCI};
    const uint16_t VENDOR_ID_INTEL = 0x8086;

    const uint8_t REG_ADDR_USB3PRM = 0xdc;
    const uint8_t REG_ADDR_USB3_PSSEN = 0xd8;
    const uint8_t REG_ADDR_XUSB2PRM = 0xd4;
    const uint8_t REG_ADDR_XUSB2PR = 0xd0;

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
    uint16_t ReadVendorId(Device device);
    uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
    ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);
    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);
    WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index);

    uint32_t ReadConfReg(const Device& dev, uint8_t reg_addr);
    void WriteConfReg(const Device& dev, uint8_t reg_addr, uint32_t value);

    bool IsSingleFunctionDevice(uint8_t header_type);

    union CapabilityHeader {
        uint32_t data;
        struct {
            uint32_t cap_id : 8;
            uint32_t next_ptr : 8;
            uint32_t cap : 16;
        } __attribute__((packed)) bits;
    } __attribute__((packed));

    // Capability ID
    const uint8_t kCapabilityMSI = 0x05;
    const uint8_t kCapabilityMSIX = 0x11;

    CapabilityHeader ReadCapabilityHeader(const Device& dev, uint8_t addr);

    struct MSICapability {
        union {
            uint32_t data;
            struct {
                uint32_t cap_id : 8;
                uint32_t next_ptr : 8;
                uint32_t msi_enable : 1;
                uint32_t multi_msg_capable : 3;
                uint32_t multi_msg_enable : 3;
                uint32_t addr_64_capable : 1;
                uint32_t per_vector_mask_capable : 1;
                uint32_t : 7;
            } __attribute__((packed)) bits;
        } __attribute__((packed)) header;

        uint32_t msg_addr;
        uint32_t msg_upper_addr;
        uint32_t msg_data;
        uint32_t mask_bits;
        uint32_t pending_bits;
    } __attribute__((packed));

    Error ConfigureMSI(const Device& dev, uint32_t msg_addr, uint32_t msg_data,
                       unsigned int num_vector_exponent);

    enum class MSITriggerMode {
        kEdge = 0,
        kLevel = 1
    };

    enum class MSIDeliveryMode {
        kFixed = 0b000,
        kLowestPriority = 0b001,
        kSMI = 0b010,
        kNMI = 0b100,
        kINIT = 0b101,
        kExtINT = 0b111,
    };

    Error ConfigureMSIFixedDestination(
        const Device& dev, uint8_t apic_id, MSITriggerMode trigger_mode,
        MSIDeliveryMode delivery_mode, uint8_t vector, unsigned int num_vector_exponent);
};
