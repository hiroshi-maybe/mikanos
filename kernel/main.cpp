#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "console.hpp"
#include "frame_buffer_config.hpp"
#include "font.hpp"
#include "graphics.hpp"

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
  "@              ",
  "@@             ",
  "@.@            ",
  "@..@           ",
  "@...@          ",
  "@....@         ",
  "@.....@        ",
  "@......@       ",
  "@.......@      ",
  "@........@     ",
  "@.........@    ",
  "@..........@   ",
  "@...........@  ",
  "@............@ ",
  "@......@@@@@@@@",
  "@......@       ",
  "@....@@.@      ",
  "@...@ @.@      ",
  "@..@   @.@     ",
  "@.@    @.@     ",
  "@@      @.@    ",
  "@       @.@    ",
  "         @.@   ",
  "         @@@   ",
};

void* operator new(size_t size, void* buf) {
    return buf;
}
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

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;
extern "C" void KernelMain(FrameBufferConfig& frame_buffer_config) {
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

    console = new(console_buf) Console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

    for (int i = 0; i < 27; ++i) {
        printk("printk: %d\n", i);
    }

    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] == '@') {
                pixel_writer->Write(200 + dx, 100 + dy, {0, 0, 0});
            } else if (mouse_cursor_shape[dy][dx] == '.') {
                pixel_writer->Write(200 + dx, 100 + dy, {255, 255, 255});
            }
        }
    }

    while(1) __asm__("hlt");
}
