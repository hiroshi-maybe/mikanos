#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "asmfunc.h"
#include "console.hpp"
#include "frame_buffer_config.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include "interrupt.hpp"
#include "logger.hpp"
#include "memory_map.hpp"
#include "mouse.hpp"
#include "pci.hpp"
#include "queue.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};
const uint32_t LOCAL_APIC_ID_REG_ADDR = 0xfee00020;

void operator delete(void* obj) noexcept {}

char console_buf[sizeof(Console)];
Console* console;
int printk(const char* format, ...) {
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    console->PutString(s);
    return result;
}

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor* mouse_cursor;
void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
    mouse_cursor->MoveRelative({displacement_x, displacement_y});
}

void SwitchEhci2Xhci(const pci::Device& xhc_dev) {
    bool intel_ehc_exist = false;
    for (int i=0; i < pci::num_device; ++i) {
        if (pci::devices[i].class_code.Match(pci::CLASSCODE_EHCI) &&
            pci::ReadVendorId(pci::devices[i]) == pci::VENDOR_ID_INTEL)
        {
            intel_ehc_exist = true;
            break;
        }
    }

    if(!intel_ehc_exist) return;

    uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, pci::REG_ADDR_USB3PRM);
    pci::WriteConfReg(xhc_dev, pci::REG_ADDR_USB3_PSSEN, superspeed_ports);
    uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, pci::REG_ADDR_XUSB2PRM);
    pci::WriteConfReg(xhc_dev, pci::REG_ADDR_XUSB2PR, ehci2xhci_ports);
    Log(kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n", superspeed_ports, ehci2xhci_ports);
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

usb::xhci::Controller* xhc;

struct Message {
    enum Type {
        kInterruptXHCI,
    } type;
};

ArrayQueue<Message>* main_queue;

__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame* frame) {
    Log(kDebug, "Interrupt happened\n");
    main_queue->Push(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
}

void onInterruptXHCIMessage() {
    while (xhc->PrimaryEventRing()->HasFront()) {
        if (auto err = ProcessEvent(*xhc)) {
            Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(), err.File(), err.Line());
        }
    }
}

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(
    const FrameBufferConfig& frame_buffer_config_ref, const MemoryMap& memory_map_ref)
{
    FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
    MemoryMap memory_map{memory_map_ref};

    switch  (frame_buffer_config.pixel_format) {
        case kPixelRGBResv8BitPerColor:
            pixel_writer = new(pixel_writer_buf)
                RGBResv8BitPerColorPixelWriter{frame_buffer_config};
            break;
        case kPixelBGRResv8BitPerColor:
            pixel_writer = new(pixel_writer_buf)
                BGRResv8BitPerColorPixelWriter{frame_buffer_config};
            break;
    }

    const int kFrameWidth = frame_buffer_config.horizontal_resolution;
    const int kFrameHeight = frame_buffer_config.vertical_resolution;

    FillRectangle(*pixel_writer, {0, 0}, {kFrameWidth, kFrameHeight - 50},
        kDesktopBGColor);
    FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth, 50},
        {1, 8, 17});
    FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth / 5, 50},
        {80, 80, 80});
    DrawRectangle(*pixel_writer, {10, kFrameHeight - 40}, {30, 30}, {160, 160, 160});

    console = new(console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};
    SetLogLevel(kInfo);

    const std::array available_memory_types{
        MemoryType::kEfiBootServicesCode,
        MemoryType::kEfiBootServicesData,
        MemoryType::kEfiConventionalMemory,
    };

    for (uintptr_t iter = reinterpret_cast<uintptr_t>(memory_map.buffer);
        iter < reinterpret_cast<uintptr_t>(memory_map.buffer) + memory_map.map_size;
        iter += memory_map.descriptor_size)
    {
        auto desc = reinterpret_cast<MemoryDescriptor*>(iter);
        for (int i = 0; i < available_memory_types.size(); ++i) {
            if (desc->type == available_memory_types[i]) {
                printk("type = %u, phy = %08lx - %08lx, pages = %lu, attr = %08lx\n",
                    desc->type, desc->physical_start,
                    desc->physical_start + desc->number_of_pages * kUEFIPageSize - 1,
                    desc->number_of_pages, desc->attribute);
            }
        }
    }

    mouse_cursor = new(mouse_cursor_buf) MouseCursor{
        pixel_writer, kDesktopBGColor, {300, 200}
    };

    std::array<Message, 32> main_queue_data;
    ArrayQueue<Message> main_queue{main_queue_data};
    ::main_queue = &main_queue;

    auto err = pci::ScanAllBus();
    Log(kDebug, "ScanAllBus: %s (%d devices)\n", err.Name(), pci::num_device);
    for (int i = 0; i < pci::num_device; ++i) {
        const auto& dev = pci::devices[i];
        auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
        auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
        Log(kDebug, "%d.%d.%d: vend %04x, class (%02x, %02x, %02x), head %02x\n",
            dev.bus, dev.device, dev.function, vendor_id, class_code.base, class_code.sub, class_code.interface, dev.header_type);
    }

    pci::Device* xhc_dev = nullptr;
    for (int i = 0; i < pci::num_device; ++i) {
        if (pci::devices[i].class_code.Match(pci::CLASSCODE_EHCI)) {
            xhc_dev = &pci::devices[i];

            if (pci::ReadVendorId(*xhc_dev) == pci::VENDOR_ID_INTEL) break;
        }
    }

    if (xhc_dev) {
        Log(kInfo, "xHC has been found: %d.%d.%d\n",
            xhc_dev->bus, xhc_dev->device, xhc_dev->function);
    }

    const uint16_t cs = GetCS();
    SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);
    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

    const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;
    pci::ConfigureMSIFixedDestination(
        *xhc_dev, bsp_local_apic_id, pci::MSITriggerMode::kLevel,
        pci::MSIDeliveryMode::kFixed, InterruptVector::kXHCI, 0);

    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
    Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
    // bitwise & with 0xfffffffffffffff0 (64 bit integer)
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

    usb::xhci::Controller xhc{xhc_mmio_base};
    if (pci::ReadVendorId(*xhc_dev) == pci::VENDOR_ID_INTEL) {
        SwitchEhci2Xhci(*xhc_dev);
    }

    {
        auto err = xhc.Initialize();
        Log(kDebug, "xhc.Initialize: %s\n", err.Name());
    }

    Log(kInfo, "xHC starting\n");
    xhc.Run();

    ::xhc = &xhc;

    usb::HIDMouseDriver::default_observer = MouseObserver;

    for (int i = 0; i <= xhc.MaxPorts(); ++i) {
        auto port = xhc.PortAt(i);
        Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

        if (port.IsConnected()) {
            if (auto err = ConfigurePort(xhc, port)) {
                Log(kError, "failed to configure port; %s at %s:%d\n", err.Name(), err.File(), err.Line());
                continue;
            }
        }
    }

    while (true) {
        __asm__("cli");
        if (main_queue.Count() == 0) {
            __asm__("sti\n\thlt");
            continue;
        }

        Message msg = main_queue.Front();
        main_queue.Pop();
        __asm__("sti");

        switch (msg.type) {
        case Message::kInterruptXHCI:
            onInterruptXHCIMessage();
            break;
        default:
            Log(kError, "Unknown message type: %d\n", msg.type);
        }
    }

    while(1) __asm__("hlt");
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}
