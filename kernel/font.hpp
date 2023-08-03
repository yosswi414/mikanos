#pragma once

#include <cstdint>
#include "graphics.hpp"

#define KERNEL_GLYPH_HEIGHT (19)
#define KERNEL_GLYPH_WIDTH  (8)
#define KERNEL_TAB_WIDTH    (4)

void WriteAscii(PixelWriter& writer, Vector2D<int> pos, char c, const PixelColor& color);
void WriteString(PixelWriter& writer, Vector2D<int> pos, const char* s, const PixelColor& color);
