#include "usb/xhci/xhci.hpp"

namespace usb::xhci {

Controller::Controller(uintptr_t mmio_base):
    mmio_base_{mmio_base},
    cap_{reinterpret_cast<CapabilityRegisters*>(mmio_base)},
    op_{reinterpret_cast<OperationalRegisters*>(
        mmio_base + cap_->CAPLENGTH.Read())},
    max_ports_{static_cast<uint8_t>(
        cap_->HCSPARAMS1.Read().bits.max_ports)}
{}

}