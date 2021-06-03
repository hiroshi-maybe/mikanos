#pragma once

#include "error.hpp"
#include "usb/xhci/registers.hpp"

namespace usb::xhci {
class Controller {
public:
    Controller(uintptr_t mmio_base);
    Error Initialize();
private:
    const uintptr_t mmio_base_;
    CapabilityRegisters* const cap_;
    OperationalRegisters* const op_;
    const uint8_t max_ports_;
};
}
