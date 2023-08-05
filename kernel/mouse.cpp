
#include "mouse.hpp"

#include "graphics.hpp"

namespace {
    // #@@range_begin(mouse_cursor)
    // const int kMouseCursorWidth = 15;
    // const int kMouseCursorHeight = 24;
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
}  // namespace
// #@@range_begin(draw_cursor)
void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position) {
    for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
        for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
            PixelColor c;
            switch (mouse_cursor_shape[dy][dx]) {
                case '@':
                    c = {0, 0, 0};
                    break;
                case '.':
                    c = {255, 255, 255};
                    break;
                default:
                    c = kMouseTransparentColor;
                    break;
            }
            pixel_writer->Write(position + Vector2D<int>{dx, dy}, c);
        }
    }
}
// #@@range_end(draw_cursor)
