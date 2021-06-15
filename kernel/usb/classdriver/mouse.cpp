#include "usb/classdriver/mouse.hpp"

#include "logger.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"

namespace usb {

HIDMouseDriver::HIDMouseDriver(Device* dev, int interface_index)
    : HIDBaseDriver{dev, interface_index, 3}
{}

Error HIDMouseDriver::OnDataReceived() {
    int8_t displacement_x = Buffer()[1];
    int8_t displacement_y = Buffer()[2];
    NotifyMouseMove(displacement_x, displacement_y);
    Log(kDebug, "%02x,(%03d,%03d)\n", Buffer()[0], displacement_x, displacement_y);
}

void* HIDMouseDriver::operator new(size_t size) {
    return AllocMem(sizeof(HIDMouseDriver), 0, 0);
}

void HIDMouseDriver::operator delete(void* ptr) noexcept {
    FreeMem(ptr);
}

void HIDMouseDriver::SubscribeMouseMove(std::function<HIDMouseDriver::ObserverType> observer) {
    observers_[num_observers_++] = observer;
}

std::function<HIDMouseDriver::ObserverType> HIDMouseDriver::default_observer;

void HIDMouseDriver::NotifyMouseMove(int8_t displacement_x, int8_t displacement_y) {
    for (int i = 0; i < num_observers_; ++i) {
        observers_[i](displacement_x, displacement_y);
    }
}

}
