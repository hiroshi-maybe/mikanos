#include <cstring>

#include "console.hpp"
#include "font.hpp"
#include "layer.hpp"

Console::Console(const PixelColor& fg_color, const PixelColor& bg_color)
    :  fg_color_{fg_color}, bg_color_{bg_color}, buffer_{},
        cursor_row_{0}, cursor_column_{0}
{}

void Console::PutString(const char* s) {
    while (*s) {
        if (*s == '\n') {
            cursor_column_ = 0;
            Newline();
        } else if (cursor_column_ < kColumns - 1) {
            WriteAscii(*writer_, Vector2D<int>{PIXEL_WIDTH_PER_CHAR * cursor_column_, PIXEL_HEIGHT_PER_CHAR * cursor_row_}, *s, fg_color_);
            buffer_[cursor_row_][cursor_column_] = *s;
            ++cursor_column_;
        }
        ++s;
    }

    if (layer_manager) layer_manager->Draw();
}

void Console::SetWriter(PixelWriter* writer) {
    if (writer == writer_) return;
    writer_ = writer;
    Refresh();
}

void Console::SetWindow(const std::shared_ptr<Window>& window) {
    if (window == window_) return;
    window_ = window;
    writer_ = window->Writer();
    Refresh();
}

void Console::Newline() {
    if (cursor_row_ < kRows - 1) {
        ++cursor_row_;
        return;
    }

    if (window_) {
        Vector2D<int> pos_to_move = {0, PIXEL_HEIGHT_PER_CHAR};
        Vector2D<int> size_to_move = {PIXEL_WIDTH_PER_CHAR * kColumns, PIXEL_HEIGHT_PER_CHAR * (kRows - 1)};
        Rectangle<int> move_src{pos_to_move, size_to_move};
        Vector2D<int> dest_pos = {0, 0};
        window_->Move(dest_pos, move_src);
        // Paint the back color of the last row
        FillRectangle(*writer_, {0, PIXEL_HEIGHT_PER_CHAR * (kRows - 1)}, {PIXEL_WIDTH_PER_CHAR * kColumns, PIXEL_HEIGHT_PER_CHAR}, bg_color_);
    } else {
        FillRectangle(*writer_, {0, 0}, {PIXEL_WIDTH_PER_CHAR * kColumns, PIXEL_HEIGHT_PER_CHAR * kRows}, bg_color_);
        for (int row = 0; row < kRows - 1; ++row) {
            memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
            WriteString(*writer_, Vector2D<int>{0, PIXEL_HEIGHT_PER_CHAR * row}, buffer_[row], fg_color_);
        }
        memset(buffer_[kRows - 1], 0, kColumns + 1);
    }
}

void Console::Refresh() {
    for (int row = 0; row < kRows; ++row) {
        WriteString(*writer_, Vector2D<int>{0, PIXEL_HEIGHT_PER_CHAR * row}, buffer_[row], fg_color_);
    }
}
