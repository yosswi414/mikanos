
#include "mouse.hpp"

#include <limits>
#include <memory>

#include "graphics.hpp"
#include "layer.hpp"
#include "usb/classdriver/mouse.hpp"

namespace {
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
}  // namespace

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

Mouse::Mouse(unsigned int layer_id) : layer_id_{layer_id} {}

void Mouse::SetPosition(Vector2D<int> position) {
    position_ = position;
    layer_manager->Move(layer_id_, position_);
}

void Mouse::OnInterrupt(uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
    const auto oldpos = position_;
    auto newpos = position_ + Vector2D<int>{displacement_x, displacement_y};
    newpos = ElementMin(newpos, ScreenSize() + Vector2D<int>{-1, -1});
    newpos = ElementMax(newpos, {0, 0});
    position_ = newpos;

    const auto posdiff = position_ - oldpos;

    layer_manager->Move(layer_id_, position_);

    const bool previous_left_pressed = previous_buttons_ & kMouseKeyLeft;
    const bool left_pressed = buttons & kMouseKeyLeft;
    if (!previous_left_pressed && left_pressed) {
        auto layer = layer_manager->FindLayerByPosition(position_, layer_id_);
        if (layer && layer->IsDraggable()) {
            drag_layer_id_ = layer->ID();
            // クリックしたレイヤを最前面にしたい
            layer_manager->SetToFront(drag_layer_id_);
        }
    } else if (previous_left_pressed && left_pressed) {
        if (drag_layer_id_ > 0) layer_manager->MoveRelative(drag_layer_id_, posdiff);
    } else if (previous_left_pressed && !left_pressed) {
        drag_layer_id_ = 0;
    }

    previous_buttons_ = buttons;
}

void InitializeMouse() {
    auto mouse_window = std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight, screen_config.pixel_format);
    mouse_window->SetTransparentColor(kMouseTransparentColor);
    DrawMouseCursor(mouse_window.get(), {0, 0});

    auto mouse_layer_id = layer_manager->NewLayer()
                              .SetWindow(mouse_window)
                              .ID();

    auto mouse = std::make_shared<Mouse>(mouse_layer_id);
    mouse->SetPosition({200, 200});
    layer_manager->UpDown(mouse->LayerID(), std::numeric_limits<int>::max());

    usb::HIDMouseDriver::default_observer = [mouse](uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
        mouse->OnInterrupt(buttons, displacement_x, displacement_y);
    };
}