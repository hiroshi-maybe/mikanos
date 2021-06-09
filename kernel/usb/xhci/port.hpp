#pragma once

#include <cstdint>

#include "error.hpp"
#include "usb/xhci/registers.hpp"

namespace usb::xhci {

class Port {
public:
    Port(uint8_t port_num, PortRegisterSet& port_reg_set):
        port_num_{port_num},
        port_reg_set_{port_reg_set}
    {}

    uint8_t Number() const;
    bool IsConnected() const;
    Error Reset();
private:
    const uint8_t port_num_;
    PortRegisterSet& port_reg_set_;
};

}