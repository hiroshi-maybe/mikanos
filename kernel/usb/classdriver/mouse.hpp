#pragma once

#include <functional>
#include "usb/classdriver/hid.hpp"

namespace usb {

class HIDMouseDriver: public HIDBaseDriver {
public:
    HIDMouseDriver(Device* dev, int interface_index);

    void* operator new(size_t size);
    void operator delete(void* ptr) noexcept;

    using ObserverType = void (int8_t displacement_x, int8_t displacement_y);
    static std::function<ObserverType> default_observer;
};

}
