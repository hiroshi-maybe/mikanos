#include "usb/classdriver/hid.hpp"

namespace usb {

HIDBaseDriver::HIDBaseDriver(Device* dev, int interface_index,
                             int in_packet_size):
    ClassDriver{dev},
    interface_index_{interface_index},
    in_packet_size_{in_packet_size}
{}

}