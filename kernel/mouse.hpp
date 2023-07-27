#pragma once

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

void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position);
