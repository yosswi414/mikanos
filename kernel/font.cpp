/**
 * @file font.cpp
 *
 * フォント描画のプログラムを集めたファイル.
 */

#include "font.hpp"

// extern const uint8_t _binary_hankaku_bin_start;
// extern const uint8_t _binary_hankaku_bin_end;
// extern const uint8_t _binary_hankaku_bin_size;

extern uint8_t* font_data_start;
extern uint64_t font_data_size;

// const uint8_t* GetFont(char c) {
//   auto index = KERNEL_GLYPH_HEIGHT * static_cast<unsigned int>(c);
//   if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
//     return nullptr;
//   }
//   return &_binary_hankaku_bin_start + index;
// }

const uint8_t* GetFont(char c) {
  auto index = KERNEL_GLYPH_HEIGHT * static_cast<unsigned int>(c);
  if (index >= font_data_size) {
    return nullptr;
  }
  return font_data_start + index;
}

void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color) {
  const uint8_t* font = GetFont(c);
  if (font == nullptr) {
    return;
  }
  for (int dy = 0; dy < KERNEL_GLYPH_HEIGHT; ++dy) {
    for (int dx = 0; dx < KERNEL_GLYPH_WIDTH; ++dx) {
      if ((font[dy] >> dx) & 0x1u) {
        writer.Write(x + dx, y + dy, color);
      }
    }
  }
}

// #@@range_begin(write_string)
void WriteString(PixelWriter& writer, int x, int y, const char* s, const PixelColor& color) {
  for (int i = 0; s[i] != '\0'; ++i) {
    WriteAscii(writer, x + KERNEL_GLYPH_WIDTH * i, y, s[i], color);
  }
}
// #@@range_end(write_string)
