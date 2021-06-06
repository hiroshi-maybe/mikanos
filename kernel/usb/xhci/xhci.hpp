#pragma once

#include "error.hpp"
#include "usb/xhci/devmgr.hpp"
#include "usb/xhci/registers.hpp"
#include "usb/xhci/ring.hpp"

namespace usb::xhci {
class Controller {
public:
    Controller(uintptr_t mmio_base);
    Error Initialize();
private:
    static const size_t kDeviceSize = 8;
    const uintptr_t mmio_base_;
    CapabilityRegisters* const cap_;
    OperationalRegisters* const op_;
    const uint8_t max_ports_;

    class DeviceManager devmgr_;
    Ring cr_;
    EventRing er_;

    InterrupterRegisterSetArray InterrupterRegisterSets() const {
        return {mmio_base_ + cap_->RTSOFF.Read().Offset() + 0x20u, 1024};
    }
};
}
