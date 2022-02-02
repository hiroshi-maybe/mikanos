#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <deque>

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
#include "segment.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"

void operator delete(void* obj) noexcept {}

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

std::deque<Message>* main_queue;

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(
    const FrameBufferConfig& frame_buffer_config_ref, const MemoryMap& memory_map_ref)
{
    screen_config = frame_buffer_config_ref;
    MemoryMap memory_map{memory_map_ref};

    InitializeGraphics(frame_buffer_config_ref);
    InitializeConsole();
    printk("Welcome to MikanOS!\n");
    SetLogLevel(kInfo);

    InitializeSegmentation();
    InitializePaging();
    InitializeMemoryManager(memory_map);

    ::main_queue = new std::deque<Message>(32);

    InitializePCI();
    InitializeInterrupt(main_queue);
    
    usb::xhci::Initialize();

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
        if (main_queue->size() == 0) {
            __asm__("sti");
            continue;
        }

        Message msg = main_queue->front();
        main_queue->pop_front();
        __asm__("sti");

        switch (msg.type) {
        case Message::kInterruptXHCI:
            usb::xhci::ProcessEvents();
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
