#include "mouse.hpp"

#include "layer.hpp"
#include "logger.hpp"
#include "usb/classdriver/mouse.hpp"

namespace {

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

// bit position 0 is for left button (1: right button, 2: center button)
const int LEFT_MOUSE_BUTTON_MASK = 0x01;

}

void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            if (mouse_cursor_shape[dy][dx] == '@') {
                pixel_writer->Write(position + Vector2D<int>{dx, dy}, {0, 0, 0});
            } else if (mouse_cursor_shape[dy][dx] == '.') {
                pixel_writer->Write(position + Vector2D<int>{dx, dy}, {255, 255, 255});
            } else {
                pixel_writer->Write(position + Vector2D<int>{dx, dy}, kMouseTransparentColor);
            }
        }
    }
}

Mouse::Mouse(unsigned int layer_id) : layer_id_{layer_id} {}

void Mouse::SetPosition(Vector2D<int> position) {
    position_ = position;
    layer_manager->Move(layer_id_, position_);
}

void Mouse::OnInterrupt(uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
    const auto oldpos = position_;
    auto new_pos = position_ + Vector2D<int>{displacement_x, displacement_y};
    new_pos = ElementMin(new_pos, ScreenSize() + Vector2D<int>{-1, -1});
    position_ = ElementMax(new_pos, {0, 0});

    const auto posdiff = position_ - oldpos;

    layer_manager->Move(layer_id_, position_);

    const bool previous_left_pressed = (previous_buttons_ & LEFT_MOUSE_BUTTON_MASK);
    const bool left_pressed = (buttons & LEFT_MOUSE_BUTTON_MASK);

    if (!previous_left_pressed && left_pressed) {
        Log(kDebug, "has pressed left: %d\n", layer_id_);
        // started dragging
        auto layer = layer_manager->FindLayerByPosition(position_, layer_id_);
        if (layer && layer->IsDraggable()) drag_layer_id_ = layer->ID();
    } else if (previous_left_pressed && left_pressed) {
        Log(kDebug, "keep left pressed: %d\n", drag_layer_id_);
        // keep dragging
        if (drag_layer_id_ > 0) layer_manager->MoveRelative(drag_layer_id_, posdiff);
    } else if (previous_left_pressed && !left_pressed) {
        // stopped dragging
        Log(kDebug, "Stopped pressing left: %d\n", drag_layer_id_);
        drag_layer_id_ = 0;
    }

    previous_buttons_ = buttons;
}

std::shared_ptr<Mouse> MakeMouse() {
    auto mouse_window =
        std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight, screen_config.pixel_format);
    mouse_window->SetTransparentColor(kMouseTransparentColor);
    DrawMouseCursor(mouse_window->Writer(), {0, 0});

    auto mouse_layer_id = layer_manager->NewLayer()
        .SetWindow(mouse_window)
        .ID();

    auto mouse = std::make_shared<Mouse>(mouse_layer_id);
    mouse->SetPosition({200, 200});
    usb::HIDMouseDriver::default_observer = [mouse](uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
        mouse->OnInterrupt(buttons, displacement_x, displacement_y);
    };

    return mouse;
}
