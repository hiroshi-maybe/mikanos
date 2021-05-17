#pragma once

#include <cstdint>
#include "graphics.hpp"

const int PIXEL_HEIGHT_PER_CHAR = 16;
const int PIXEL_WIDTH_PER_CHAR = 8;

void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color);
void WriteString(PixelWriter& writer, int x, int y, const char* s, const PixelColor& color);