#pragma once

#include "usb/classdriver/base.hpp"

namespace usb {

class HIDBaseDriver: public ClassDriver {
public:
    HIDBaseDriver(Device* dev, int interface_index, int in_packet_size);
private:
    const int interface_index_;
    int in_packet_size_;
};

}