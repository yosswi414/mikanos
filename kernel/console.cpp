/**
 * @file console.cpp
 *
 * コンソール描画のプログラムを集めたファイル．
 */

#include "console.hpp"

#include <cstring>
#include "font.hpp"

// #@@range_begin(constructor)
Console::Console(PixelWriter& writer,
    const PixelColor& fg_color, const PixelColor& bg_color)
    : writer_{writer}, fg_color_{fg_color}, bg_color_{bg_color},
      buffer_{}, cursor_row_{0}, cursor_column_{0} {
}
// #@@range_end(constructor)

// #@@range_begin(put_string)
void Console::PutString(const char* s) {
  while (*s) {
    if (*s == '\n') {
      Newline();
    } else if(*s == '\t'){ // this implementation could be naive
      do {
        if (cursor_column_ >= kColumns) {
          Newline();
          break;
        }
        buffer_[cursor_row_][cursor_column_++] = ' ';
      } while (cursor_column_ % KERNEL_TAB_WIDTH > 0);
    } else if (cursor_column_ < kColumns - 1) {
      WriteAscii(writer_, KERNEL_GLYPH_WIDTH * cursor_column_, KERNEL_GLYPH_HEIGHT * cursor_row_, *s, fg_color_);
      buffer_[cursor_row_][cursor_column_] = *s;
      ++cursor_column_;
    }
    ++s;
  }
}
// #@@range_end(put_string)

// #@@range_begin(newline)
void Console::Newline() {
  cursor_column_ = 0;
  if (cursor_row_ < kRows - 1) {
    ++cursor_row_;
  } else {
    for (int y = 0; y < KERNEL_GLYPH_HEIGHT * kRows; ++y) {
      for (int x = 0; x < KERNEL_GLYPH_WIDTH * kColumns; ++x) {
        writer_.Write(x, y, bg_color_);
      }
    }
    for (int row = 0; row < kRows - 1; ++row) {
      memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
      WriteString(writer_, 0, KERNEL_GLYPH_HEIGHT * row, buffer_[row], fg_color_);
    }
    memset(buffer_[kRows - 1], 0, kColumns + 1);
  }
}
// #@@range_end(newline)
