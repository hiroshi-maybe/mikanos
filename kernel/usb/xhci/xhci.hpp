#pragma once

#include "error.hpp"
#include "usb/xhci/devmgr.hpp"
#include "usb/xhci/port.hpp"
#include "usb/xhci/registers.hpp"
#include "usb/xhci/ring.hpp"

namespace usb::xhci {
class Controller {
public:
    Controller(uintptr_t mmio_base);
    Error Initialize();
    Error Run();
    Port PortAt(uint8_t port_num) {
        return Port{port_num, PortRegisterSets()[port_num - 1]};
    }
    uint8_t MaxPorts() const { return max_ports_; }
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

    PortRegisterSetArray PortRegisterSets() const {
        return {reinterpret_cast<uintptr_t>(op_) + 0x400u, max_ports_};
    }
};

Error ConfigurePort(Controller& xhc, Port& port);

}
