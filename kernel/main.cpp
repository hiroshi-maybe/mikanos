#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "asmfunc.h"
#include "console.hpp"
#include "frame_buffer_config.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include "interrupt.hpp"
#include "layer.hpp"
#include "logger.hpp"
#include "memory_manager.hpp"
#include "memory_map.hpp"
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "queue.hpp"
#include "segment.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"

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

char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager* memory_manager;

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
    screen_config = frame_buffer_config_ref;
    MemoryMap memory_map{memory_map_ref};

    switch  (screen_config.pixel_format) {
        case kPixelRGBResv8BitPerColor:
            pixel_writer = new(pixel_writer_buf)
                RGBResv8BitPerColorPixelWriter{screen_config};
            break;
        case kPixelBGRResv8BitPerColor:
            pixel_writer = new(pixel_writer_buf)
                BGRResv8BitPerColorPixelWriter{screen_config};
            break;
    }

    DrawDesktop(*pixel_writer);

    console = new(console_buf) Console{kDesktopFGColor, kDesktopBGColor};
    console->SetWriter(pixel_writer);
    printk("Welcome to MikanOS!\n");
    SetLogLevel(kInfo);

    SetupSegments();

    const uint16_t kernel_cs = 1 << 3;
    const uint16_t kernel_ss = 2 << 3;
    SetDSAll(0); // Point segment registers to the null descriptor
    SetCSSS(kernel_cs, kernel_ss);

    SetupIdentityPageTable();

    ::memory_manager = new(memory_manager_buf) BitmapMemoryManager;

    const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
    uintptr_t available_end = 0;
    for (uintptr_t iter = memory_map_base;
        iter < memory_map_base + memory_map.map_size;
        iter += memory_map.descriptor_size)
    {
        auto desc = reinterpret_cast<const MemoryDescriptor*>(iter);
        if (available_end < desc->physical_start) {
            memory_manager->MarkAllocated(
                FrameID{available_end / kBytesPerFrame},
                (desc->physical_start - available_end) / kBytesPerFrame);
        }

        const auto physical_end = desc->physical_start + desc->number_of_pages * kUEFIPageSize;
        if (IsAvailable(static_cast<MemoryType>(desc->type))) {
            Log(kDebug, "type = %u, phy = %08lx - %08lx, pages = %lu, attr = %08lx\n",
                desc->type, desc->physical_start,
                desc->physical_start + desc->number_of_pages * kUEFIPageSize - 1,
                desc->number_of_pages, desc->attribute);
            available_end = physical_end;
        } else {
            memory_manager->MarkAllocated(
                FrameID{desc->physical_start / kBytesPerFrame},
                desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
        }
    }
    memory_manager->SetMemoryRange(FrameID{1}, FrameID{available_end / kBytesPerFrame});

    if (auto err = InitializeHeap(*memory_manager)) {
        Log(kError, "failed to allocate pages: %s at %s:%d for heap\n",
            err.Name(), err.File(), err.Line());
        exit(1);
    }

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

    SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                reinterpret_cast<uint64_t>(IntHandlerXHCI), kernel_cs);
    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

    const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t*>(LOCAL_APIC_ID_REG_ADDR) >> 24;
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

    const auto screen_size = ScreenSize();

    auto bgwindow =
        std::make_shared<Window>(screen_size.x, screen_size.y, screen_config.pixel_format);
    auto bgwriter = bgwindow->Writer();

    DrawDesktop(*bgwriter);

    auto main_window = std::make_shared<Window>(160, 52, screen_config.pixel_format);
    DrawWindow(*main_window->Writer(), "Hello Window");

    auto console_window = std::make_shared<Window>(
            Console::kColumns * PIXEL_WIDTH_PER_CHAR, Console::kRows * PIXEL_HEIGHT_PER_CHAR, screen_config.pixel_format);
    console->SetWindow(console_window);

    FrameBuffer screen;
    if (auto err = screen.Initialize(screen_config)) {
        Log(kError, "failed to initialize frame buffer: %s at %s:%d\n",
            err.Name(), err.File(), err.Line());
    }

    layer_manager = new LayerManager;
    layer_manager->SetWriter(&screen);

    auto mouse = MakeMouse();

    auto bglayer_id = layer_manager->NewLayer()
        .SetWindow(bgwindow)
        .Move({0, 0})
        .ID();
    auto main_window_layer_id = layer_manager->NewLayer()
        .SetWindow(main_window)
        .SetDraggable(true)
        .Move({300, 100})
        .ID();
    console->SetLayerID(layer_manager->NewLayer()
        .SetWindow(console_window)
        .Move({0, 0})
        .ID());

    layer_manager->UpDown(bglayer_id, 0);
    layer_manager->UpDown(console->LayerID(), 1);
    layer_manager->UpDown(main_window_layer_id, 2);
    layer_manager->UpDown(mouse->LayerID(), 3);
    layer_manager->Draw({{0, 0}, screen_size});

    char str[128];
    unsigned int count = 0;

    while (true) {
        ++count;
        sprintf(str, "%010u", count);
        FillRectangle(*main_window->Writer(), {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
        WriteString(*main_window->Writer(), {24, 28}, str, {0, 0, 0});
        layer_manager->Draw(main_window_layer_id);

        __asm__("cli");
        if (main_queue.Count() == 0) {
            __asm__("sti");
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
