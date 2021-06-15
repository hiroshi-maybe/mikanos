#include "usb/classdriver/hid.hpp"

#include "logger.hpp"
#include "usb/device.hpp"

namespace usb {

HIDBaseDriver::HIDBaseDriver(Device* dev, int interface_index,
                             int in_packet_size):
    ClassDriver{dev},
    interface_index_{interface_index},
    in_packet_size_{in_packet_size}
{}

Error HIDBaseDriver::SetEndpoint(const EndpointConfig& config) {
    if (config.ep_type == EndpointType::kInterrupt && config.ep_id.IsIn()) {
        ep_interrupt_in_ = config.ep_id;
    } else if (config.ep_type == EndpointType::kInterrupt && !config.ep_id.IsIn()) {
        ep_interrupt_out_ = config.ep_id;
    }
    return MAKE_ERROR(Error::kSuccess);
}

Error HIDBaseDriver::OnControlCompleted(EndpointID ep_id, SetupData setup_data,
                                        const void* buf, int len)
{
    Log(kDebug, "HIDBaseDriver::OnControlCompleted: dev %08x, phase = %d, len = %d\n",
        this, initialize_phase_, len);
    if (initialize_phase_ == 1) {
        initialize_phase_ = 2;
        return ParentDevice()->InterruptIn(ep_interrupt_in_, buf_.data(), in_packet_size_);
    }

    return MAKE_ERROR(Error::kNotImplemented);
}

Error HIDBaseDriver::OnInterruptCompleted(EndpointID ep_id, const void* buf, int len) {
    if (ep_id.IsIn()) {
        OnDataReceived();
        std::copy_n(buf_.begin(), len, previous_buf_.begin());
        return ParentDevice()->InterruptIn(ep_interrupt_in_, buf_.data(), in_packet_size_);
    }

    return MAKE_ERROR(Error::kNotImplemented);
}

}