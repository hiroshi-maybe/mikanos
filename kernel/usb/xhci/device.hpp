#pragma once

#include <cstddef>
#include <cstdint>

#include "usb/device.hpp"

namespace usb::xhci {

class Device : public usb::Device {
public:
    enum class State {
        kInvalid,
        kBlank,
        kSlotAssigning,
        kSlotAssigned,
    };
};

}
