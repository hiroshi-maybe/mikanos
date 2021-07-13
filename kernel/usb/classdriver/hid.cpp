#include "usb/classdriver/hid.hpp"

#include "logger.hpp"
#include "usb/device.hpp"

namespace usb {

HIDBaseDriver::HIDBaseDriver(Device* dev, int interface_index,
                             int in_packet_size):
    ClassDriver{dev},
    in_packet_size_{in_packet_size},
    interface_index_{interface_index}
{}

Error HIDBaseDriver::SetEndpoint(const EndpointConfig& config) {
    if (config.ep_type == EndpointType::kInterrupt && config.ep_id.IsIn()) {
        ep_interrupt_in_ = config.ep_id;
    } else if (config.ep_type == EndpointType::kInterrupt && !config.ep_id.IsIn()) {
        ep_interrupt_out_ = config.ep_id;
    }
    return MAKE_ERROR(Error::kSuccess);
}

Error HIDBaseDriver::OnEndpointsConfigured() {
    Log(kDebug, "HIDBaseDriver::OnEndpointsConfigured\n");
    SetupData setup_data{};
    setup_data.request_type.bits.direction = request_type::kOut;
    setup_data.request_type.bits.type = request_type::kClass;
    setup_data.request_type.bits.recipient = request_type::kInterface;
    setup_data.request = request::kSetProtocol;
    setup_data.value = 0; // boot protocol
    setup_data.index = interface_index_;
    setup_data.length = 0;

    initialize_phase_ = 1;
    return ParentDevice()->ControlOut(kDefaultControlPipeID, setup_data, nullptr, 0, this);
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
    Log(kDebug, "HIDBaseDriver::OnInterruptCompleted begin\n");
    if (ep_id.IsIn()) {
        Log(kDebug, "HIDBaseDriver::OnInterruptCompleted OnDataReceived - before\n");
        OnDataReceived();
        Log(kDebug, "HIDBaseDriver::OnInterruptCompleted OnDataReceived - after\n");
        std::copy_n(buf_.begin(), len, previous_buf_.begin());
        Log(kDebug, "HIDBaseDriver::OnInterruptCompleted copy_n\n");
        return ParentDevice()->InterruptIn(ep_interrupt_in_, buf_.data(), in_packet_size_);
    }

    return MAKE_ERROR(Error::kNotImplemented);
}

}