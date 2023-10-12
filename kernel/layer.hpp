#pragma once

#include <map>
#include <memory>
#include <vector>

#include "graphics.hpp"
#include "window.hpp"

class Layer {
  public:
    /** @brief 指定された ID を持つレイヤーを生成する */
    Layer(unsigned int id = 0);
    /** @brief このインスタンスの ID を返す */
    unsigned int ID() const;

    /** @brief ウィンドウを設定する 既存のウィンドウはこのレイヤーから外れる */
    Layer& SetWindow(const std::shared_ptr<Window>& window);
    /** @brief 設定されたウィンドウを返す */
    std::shared_ptr<Window> GetWindow() const;
    /** @brief レイヤーの原点座標を取得 */
    Vector2D<int> GetPosition() const;

    /** @brief レイヤーの位置情報を指定された絶対座標へと更新する 再描画はされない */
    Layer& Move(Vector2D<int> pos);
    /** @brief レイヤーの位置情報を指定された相対座標へと更新する 再描画はされない */
    Layer& MoveRelative(Vector2D<int> pos_diff);

    /** @brief writer に現在設定されているウィンドウの内容を描画する */
    void DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const;

    Layer& SetDraggable(bool draggable);
    bool IsDraggable() const;

  private:
    unsigned int id_;
    Vector2D<int> pos_{};
    std::shared_ptr<Window> window_{};
    bool draggable_{false};
};

class LayerManager {
  public:
    /** @brief Draw メソッドなどで描画する際の描画先を設定する */
    void SetWriter(FrameBuffer* screen);
    /**
     * @brief 新しいレイヤーを生成して参照を返す
     *
     * 新しく生成されたレイヤーの実体は LayerManager 内部のコンテナで保持される
     */
    Layer& NewLayer();

    /** @brief 現在表示状態にあるレイヤーを描画する */
    void Draw(const Rectangle<int>& area) const;
    /** @brief 指定したレイヤーに設定されているウィンドウの描画領域内を再描画 */
    void Draw(unsigned int id) const;

    /** @brief レイヤーの位置情報を指定された絶対座標へと更新する 再描画も行う */
    void Move(unsigned int id, Vector2D<int> new_pos);
    /** @brief レイヤーの位置情報を指定された相対座標へと更新する 再描画も行う */
    void MoveRelative(unsigned int id, Vector2D<int> pos_diff);

    /**
     * @brief レイヤーの高さ方向の位置を指定された位置に移動する
     *
     * new_height に負の高さを指定するとレイヤーは非表示となり
     * 0 以上を指定するとその高さとなる
     * 現在のレイヤー数以上の数値を指定した場合は最前面のレイヤーとなる
     */
    void UpDown(unsigned int id, int new_height);
    /** @brief レイヤーを非表示とする */
    void Hide(unsigned int id);

    Layer* FindLayerByPosition(Vector2D<int> pos, unsigned int exclude_id) const;

    // draggable なレイヤの中で最前面に配置する
    void SetToFront(unsigned int id);

  private:
    FrameBuffer* screen_{nullptr};
    mutable FrameBuffer back_buffer_{};
    std::vector<std::unique_ptr<Layer>> layers_{};
    std::vector<Layer*> layer_stack_{};
    unsigned int latest_id_{0};

    Layer* FindLayer(unsigned int id);
};

extern LayerManager* layer_manager;
