#include "usb/xhci/xhci.hpp"

#include "logger.hpp"
#include "usb/memory.hpp"

namespace {
using namespace usb::xhci;

enum class ConfigPhase {
    kNotConnected,
    kWaitingAddressed,
    kResettingPort,
    kEnablingSlot,
    kAddressingDevice,
    kInitializingDevice,
    kConfiguringEndpoints,
    kCOnfigured,
};

// index: Port number
std::array<volatile ConfigPhase, 256> port_config_phase{};

/**
 * The number of processing port transitiong from kResettingPort to kAddressingDevice.
 * `0` represents that there is no such a port.
 */
uint8_t addressing_port{0};

Error ResetPort(Controller& xhc, Port& port) {
    const bool is_connected = port.IsConnected();
    Log(kDebug, "ResetPort: port.IsConnected() = %s\n", is_connected ? "true" : "false");
    if (!is_connected) return MAKE_ERROR(Error::kSuccess);

    if (addressing_port != 0) {
        port_config_phase[port.Number()] = ConfigPhase::kWaitingAddressed;
    } else {
        const auto port_phase = port_config_phase[port.Number()];
        if (port_phase != ConfigPhase::kNotConnected && port_phase != ConfigPhase::kWaitingAddressed) {
            return MAKE_ERROR(Error::kInvalidPhase);
        }
        addressing_port = port.Number();
        port_config_phase[port.Number()] = ConfigPhase::kResettingPort;
        port.Reset();
    }
    return MAKE_ERROR(Error::kSuccess);
}

Error RegisterCommandRing(Ring* ring, MemMapRegister<CRCR_Bitmap>* crcr) {
    CRCR_Bitmap value = crcr->Read();
    value.bits.ring_cycle_state = true;
    value.bits.command_stop = false;
    value.bits.command_abort = false;
    value.SetPointer(reinterpret_cast<uint64_t>(ring->Buffer()));
    crcr->Write(value);
    return MAKE_ERROR(Error::kSuccess);
}

void RequestHCOwnership(uintptr_t mmio_base, HCCPARAMS1_Bitmap hccp) {
    ExtendedRegisterList extregs{ mmio_base, hccp };

    auto ext_usblegsup = std::find_if(
        extregs.begin(), extregs.end(),
        [](auto& reg) { return reg.Read().bits.capability_id == 1; });

    if (ext_usblegsup == extregs.end()) return;

    auto& reg = reinterpret_cast<MemMapRegister<USBLEGSUP_Bitmap>&>(*ext_usblegsup);
    auto r = reg.Read();
    if (r.bits.hc_os_owned_semaphore) return;

    r.bits.hc_os_owned_semaphore = 1;
    Log(kDebug, "waiting until OS owns xHHC...\n");
    reg.Write(r);

    do {
        r = reg.Read();
    } while (r.bits.hc_bios_owned_semaphore
        || !r.bits.hc_os_owned_semaphore);

    Log(kDebug, "OS has owned xHC\n");
}

}

namespace usb::xhci {

Controller::Controller(uintptr_t mmio_base):
    mmio_base_{mmio_base},
    cap_{reinterpret_cast<CapabilityRegisters*>(mmio_base)},
    op_{reinterpret_cast<OperationalRegisters*>(
        mmio_base + cap_->CAPLENGTH.Read())},
    max_ports_{static_cast<uint8_t>(
        cap_->HCSPARAMS1.Read().bits.max_ports)}
{}

Error Controller::Initialize() {
    if (auto err = devmgr_.Initialize(kDeviceSize)) {
        return err;
    }

    RequestHCOwnership(mmio_base_, cap_->HCCPARAMS1.Read());

    auto usbcmd = op_->USBCMD.Read();
    usbcmd.bits.interrupter_enable = false;
    usbcmd.bits.host_system_error_enable = false;
    usbcmd.bits.enable_wrap_event = false;

    if (!op_->USBSTS.Read().bits.host_controller_halted) {
        usbcmd.bits.run_stop = false;
    }

    op_->USBCMD.Write(usbcmd);
    while (!op_->USBSTS.Read().bits.host_controller_halted);

    // Reset controller
    usbcmd = op_->USBCMD.Read();
    usbcmd.bits.host_controller_reset = true;
    op_->USBCMD.Write(usbcmd);
    while (op_->USBCMD.Read().bits.host_controller_reset);
    while (op_->USBSTS.Read().bits.controller_not_ready);

    Log(kDebug, "MaxSlots: %u\n", cap_->HCSPARAMS1.Read().bits.max_device_slots);
    auto config = op_->CONFIG.Read();
    config.bits.max_device_slots_enabled = kDeviceSize;
    op_->CONFIG.Write(config);

    auto hcsparams2 = cap_->HCSPARAMS2.Read();
    const uint16_t max_scratchpad_buffers =
        hcsparams2.bits.max_scratchpad_buffers_low
        | (hcsparams2.bits.max_scratchpad_buffers_high << 5);
    if (max_scratchpad_buffers > 0) {
        auto scratchpad_buf_arr = AllocArray<void*>(max_scratchpad_buffers, 64, 4096);
        for (int i = 0; i < max_scratchpad_buffers; ++i) {
            scratchpad_buf_arr[i] = AllocMem(4096, 4096, 4096);
            Log(kDebug, "scratch pad buffer array %d = %p\n", i, scratchpad_buf_arr[i]);
        }

        devmgr_.DeviceContexts()[0] = reinterpret_cast<DeviceContext*>(scratchpad_buf_arr);
        Log(kInfo, "wrote scratchpad buffer array %p to dev ctx array \n", scratchpad_buf_arr);
    }

    DCBAAP_Bitmap dcbaap{};
    dcbaap.SetPointer(reinterpret_cast<uint64_t>(devmgr_.DeviceContexts()));
    op_->DCBAAP.Write(dcbaap);

    auto primary_interrupter = &InterrupterRegisterSets()[0];
    if (auto err = cr_.Initialize(32)) return err;
    if (auto err = RegisterCommandRing(&cr_, &op_->CRCR)) return err;
    if (auto err = er_.Initialize(32, primary_interrupter)) return err;

    auto iman = primary_interrupter->IMAN.Read();
    iman.bits.interrupt_pending = true;
    iman.bits.interrupt_enable = true;
    primary_interrupter->IMAN.Write(iman);

    usbcmd = op_->USBCMD.Read();
    usbcmd.bits.interrupter_enable = true;
    op_->USBCMD.Write(usbcmd);

    return MAKE_ERROR(Error::kSuccess);
}

Error Controller::Run() {
    auto usbcmd = op_->USBCMD.Read();
    usbcmd.bits.run_stop = true;
    op_->USBCMD.Write(usbcmd);
    op_->USBCMD.Read();

    while (op_->USBSTS.Read().bits.host_controller_halted);

    return MAKE_ERROR(Error::kSuccess);
}

Error ConfigurePort(Controller& xhc, Port& port) {
    if (port_config_phase[port.Number()] == ConfigPhase::kNotConnected) {
        return ResetPort(xhc, port);
    }

    return MAKE_ERROR(Error::kSuccess);
}

}