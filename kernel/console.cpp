/**
 * @file console.cpp
 *
 * コンソール描画のプログラムを集めたファイル．
 */

#include "console.hpp"

#include <cstring>

#include "font.hpp"
#include "layer.hpp"

// #@@range_begin(constructor)
Console::Console(  // PixelWriter& writer,
    const PixelColor& fg_color, const PixelColor& bg_color)
    : writer_{nullptr},
      window_{},
      fg_color_{fg_color},
      bg_color_{bg_color},
      buffer_{},
      cursor_row_{0},
      cursor_column_{0} {
}
// #@@range_end(constructor)

// #@@range_begin(put_string)
void Console::PutString(const char* s) {
    while (*s) {
        if (*s == '\n') {
            Newline();
        } else if (*s == '\t') {  // this implementation could be naive
            do {
                if (cursor_column_ >= kColumns) {
                    Newline();
                    break;
                }
                buffer_[cursor_row_][cursor_column_++] = ' ';
            } while (cursor_column_ % KERNEL_TAB_WIDTH > 0);
        } else if (cursor_column_ < kColumns - 1) {
            WriteAscii(*writer_, Vector2D<int>{KERNEL_GLYPH_WIDTH * cursor_column_, KERNEL_GLYPH_HEIGHT * cursor_row_}, *s, fg_color_);
            buffer_[cursor_row_][cursor_column_] = *s;
            ++cursor_column_;
        }
        ++s;
    }
    if (layer_manager) layer_manager->Draw();
}
// #@@range_end(put_string)

void Console::SetWriter(PixelWriter* writer) {
    if (writer == writer_) return;
    writer_ = writer;
    window_.reset();
    Refresh();
}

void Console::SetWindow(const std::shared_ptr<Window>& window){
    if (window == window_) return;
    window_ = window;
    writer_ = window->Writer();
    Refresh();
}

// #@@range_begin(newline)
void Console::Newline() {
    cursor_column_ = 0;
    if (cursor_row_ < kRows - 1) {
        ++cursor_row_;
        return;
    }

    if (window_) {
        Rectangle<int> move_src{
            {0, KERNEL_GLYPH_HEIGHT},   // 1st row / 0th col
            {KERNEL_GLYPH_WIDTH * kColumns, KERNEL_GLYPH_HEIGHT * (kRows - 1)}};    // (kRows - 1) rows / kColumns cols
        window_->Move({0, 0}, move_src); // 0th row / 0th col
        FillRectangle(*writer_,
                      {0, KERNEL_GLYPH_HEIGHT * (kRows - 1)},   // (kRows - 1)th row / 0th col
                      {KERNEL_GLYPH_WIDTH * kColumns, KERNEL_GLYPH_HEIGHT}, // 1 row / kColumns cols
                      bg_color_);
    } else {
        FillRectangle(*writer_,
                      {0, 0},
                      {KERNEL_GLYPH_WIDTH * kColumns, KERNEL_GLYPH_HEIGHT * kRows},
                      bg_color_);
        for (int row = 0; row < kRows - 1; ++row) {
            memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
            WriteString(*writer_, Vector2D<int>{0, KERNEL_GLYPH_HEIGHT * row}, buffer_[row], fg_color_);
        }
        memset(buffer_[kRows - 1], 0, kColumns + 1);
    }
}
// #@@range_end(newline)

void Console::Refresh() {
    for (int row = 0; row < kRows; ++row) {
        WriteString(*writer_, {0, KERNEL_GLYPH_HEIGHT * row}, buffer_[row], fg_color_);
    }
}
