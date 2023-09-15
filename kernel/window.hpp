#pragma once

#include <optional>
#include <vector>

#include "frame_buffer.hpp"
#include "graphics.hpp"
#include "logger.hpp"

class Window : public PixelWriter {
  public:
    /** @brief 指定されたピクセル数の平面描画領域を作成する */
    Window(int width, int height, PixelFormat shadow_format);
    ~Window() = default;
    Window(const Window& rhs) = delete;
    Window& operator=(const Window& rhs) = delete;

    /**
     * @brief 与えられた PixelWriter にこのウィンドウの表示領域を描画する
     *
     * @param dst       描画先
     * @param position  writer の左上を基準とした描画位置
     */
    void DrawTo(FrameBuffer& dst, Vector2D<int> pos, const Rectangle<int>& area);
    /** @brief 透過色を設定する */
    void SetTransparentColor(std::optional<PixelColor> c);
    /** @brief PixelWriter のインタフェースである Writer() を提供 *this を返す */
    Window* Writer();

    /** @brief 指定した位置のピクセルを返す */
    const PixelColor& At(Vector2D<int> pos) const;

    /** @brief 平面描画領域の横幅をピクセル単位で返す */
    int Width() const override;
    /** @brief 平面描画領域の縦幅をピクセル単位で返す */
    int Height() const override;
    /** @brief 平面描画領域のサイズを返す */
    Vector2D<int> Size() const;

    /** @brief シャドウバッファに転送しながら描画する */
    void Write(Vector2D<int> pos, const PixelColor& c) override;

    /**
     * @brief このウィンドウの平面描画領域内で、矩形領域を移動する
     *
     * @param src.pos   移動元矩形の原点
     * @param src.size  移動元矩形の大きさ
     * @param dst_pos   移動先の原点
     */
    void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

  private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    std::optional<PixelColor> transparent_color_{std::nullopt};

    FrameBuffer shadow_buffer_{};
};

void DrawWindow(PixelWriter& writer, const char* title);
