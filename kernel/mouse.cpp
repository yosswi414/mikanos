
#include "mouse.hpp"

#include "graphics.hpp"

namespace {
    // #@@range_begin(mouse_cursor)
    const int kMouseCursorWidth = 15;
    const int kMouseCursorHeight = 24;
    const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
        "@              ",
        "@@             ",
        "@.@            ",
        "@..@           ",
        "@...@          ",  // 5
        "@....@         ",
        "@.....@        ",
        "@......@       ",
        "@.......@      ",
        "@........@     ",  // 10
        "@.........@    ",
        "@..........@   ",
        "@...........@  ",
        "@............@ ",
        "@......@@@@@@@@",  // 15
        "@......@       ",
        "@....@@.@      ",
        "@...@ @.@      ",
        "@..@   @.@     ",
        "@.@    @.@     ",  // 20
        "@@      @.@    ",
        "@       @.@    ",
        "         @.@   ",
        "         @@@   "  // 24
    };
    // #@@range_end(mouse_cursor)

    // #@@range_begin(draw_cursor)
    void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position) {
        for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
            for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
                switch (mouse_cursor_shape[dy][dx]) {
                    case '@':
                        pixel_writer->Write(position.x + dx, position.y + dy, {0, 0, 0});
                        break;
                    case '.':
                        pixel_writer->Write(position.x + dx, position.y + dy, {255, 255, 255});
                        break;
                }
            }
        }
    }
    // #@@range_end(draw_cursor)

    void EraseMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position, PixelColor erase_color) {
        for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
            for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
                if (mouse_cursor_shape[dy][dx] != ' ') {
                    pixel_writer->Write(position.x + dx, position.y + dy, erase_color);
                }
            }
        }
    }
}  // namespace

MouseCursor::MouseCursor(PixelWriter* writer, PixelColor erase_color,
                         Vector2D<int> initial_position)
    : pixel_writer_{writer},
      erase_color_{erase_color},
      position_{initial_position} {
    DrawMouseCursor(pixel_writer_, position_);
}

void MouseCursor::MoveRelative(Vector2D<int> displacement) {
    EraseMouseCursor(pixel_writer_, position_, erase_color_);
    position_ += displacement;
    DrawMouseCursor(pixel_writer_, position_);
}
