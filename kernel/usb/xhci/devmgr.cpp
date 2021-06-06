
#include "usb/xhci/devmgr.hpp"

#include "usb/memory.hpp"

namespace usb::xhci {
Error DeviceManager::Initialize(size_t max_slots) {
    max_slots_ = max_slots;

    devices_ = AllocArray<Device*>(max_slots_ + 1, 0, 0);
    if (devices_ == nullptr) return MAKE_ERROR(Error::kNoEnoughMemory);

    device_context_pointers_ = AllocArray<DeviceContext*>(max_slots_ + 1, 64, 4096);
    if (device_context_pointers_ == nullptr) {
        FreeMem(devices_);
        return MAKE_ERROR(Error::kNoEnoughMemory);
    }

    for (size_t i = 0; i <= max_slots_; ++i) {
        devices_[i] = nullptr;
        device_context_pointers_[i] = nullptr;
    }

    return MAKE_ERROR(Error::kSuccess);
}

DeviceContext** DeviceManager::DeviceContexts() const {
    return device_context_pointers_;
}

}