#include "usb/xhci/port.hpp"

namespace usb::xhci {
uint8_t Port::Number() const {
    return port_num_;
}

bool Port::IsConnected() const {
    return port_reg_set_.PORTSC.Read().bits.current_connect_status;
}

Error Port::Reset() {
    auto portsc = port_reg_set_.PORTSC.Read();
    portsc.data[0] &= 0x0e00c3e0u;
    portsc.data[0] |= 0x00020010u; // Write 1 to PR and CSC
    port_reg_set_.PORTSC.Write(portsc);
    while (port_reg_set_.PORTSC.Read().bits.port_reset);
    return MAKE_ERROR(Error::kSuccess);
}

}
