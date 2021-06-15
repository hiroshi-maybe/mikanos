#pragma once

#include <cstddef>
#include <cstdint>

#include "error.hpp"
#include "usb/arraymap.hpp"
#include "usb/device.hpp"
#include "usb/xhci/context.hpp"
#include "usb/xhci/registers.hpp"
#include "usb/xhci/ring.hpp"
#include "usb/xhci/trb.hpp"

namespace usb::xhci {

class Device : public usb::Device {
public:
    enum class State {
        kInvalid,
        kBlank,
        kSlotAssigning,
        kSlotAssigned,
    };

    Device(uint8_t slot_id, DoorbellRegister* dbreg);

    DeviceContext* DeviceContext() { return &ctx_; }
    InputContext* InputContext() { return &input_ctx_; }

    uint8_t SlotID() const { return slot_id_; }

    Ring* AllocTransferRing(DeviceContextIndex index, size_t buf_size);

    Error ControlIn(EndpointID ep_id, SetupData setup_data,
                    void* buf, int len, ClassDriver* issuer) override;
    Error ControlOut(EndpointID ep_id, SetupData setup_data,
                     const void* buf, int len, ClassDriver* issuer) override;
    Error InterruptIn(EndpointID ep_id, void* buf, int len) override;
    Error OnTransferEventReceived(const TransferEventTRB& trb);
private:
    alignas(64) struct DeviceContext ctx_;
    alignas(64) struct InputContext input_ctx_;

    const uint8_t slot_id_;
    DoorbellRegister* const dbreg_;
    std::array<Ring*, 31> transfer_rings_; // index = dci - 1
    ArrayMap<const void*, const SetupStageTRB*, 16> setup_stage_map_{};
};

}

