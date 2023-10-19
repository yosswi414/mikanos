#pragma once

#include <memory>

#include "graphics.hpp"

// class MouseCursor {
//   public:
//     MouseCursor(PixelWriter* writer, PixelColor erase_color, Vector2D<int> initial_position);
//     void MoveRelative(Vector2D<int> displacement);
//     const Vector2D<int>& getPos() { return position_; } // erase later

//   private:
//     PixelWriter* pixel_writer_ = nullptr;
//     PixelColor erase_color_;
//     Vector2D<int> position_;
// };

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const PixelColor kMouseTransparentColor{0, 0, 1};

const uint8_t kMouseKeyLeft = 0x01;
const uint8_t kMouseKeyRight = 0x02;
const uint8_t kMouseKeyMiddle = 0x04;

void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position);

class Mouse {
  public:
    Mouse(unsigned int layer_id);
    void OnInterrupt(uint8_t buttons, int8_t displacement_x, int8_t displacement_y);

    unsigned int LayerID() const { return layer_id_; }
    void SetPosition(Vector2D<int> position);
    Vector2D<int> Position() const { return position_; }

  private:
    unsigned int layer_id_;
    Vector2D<int> position_{};

    unsigned int drag_layer_id_{0};
    uint8_t previous_buttons_{0};
};

void InitializeMouse();
