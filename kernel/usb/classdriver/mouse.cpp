#include "usb/classdriver/mouse.hpp"

#include "usb/device.hpp"
#include "usb/memory.hpp"

namespace usb {

HIDMouseDriver::HIDMouseDriver(Device* dev, int interface_index)
    : HIDBaseDriver{dev, interface_index, 3}
{}

void* HIDMouseDriver::operator new(size_t size) {
    return AllocMem(sizeof(HIDMouseDriver), 0, 0);
}

void HIDMouseDriver::operator delete(void* ptr) noexcept {
    FreeMem(ptr);
}

std::function<HIDMouseDriver::ObserverType> HIDMouseDriver::default_observer;

}
