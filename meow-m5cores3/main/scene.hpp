#ifndef MEOW_SCENE_HPP
#define MEOW_SCENE_HPP

// Meow のシーン: 灰色の背景に子猫 (kitty) が座っていて、タップすると鳴く。
// ShapoGFX だけに依存し、プラットフォーム (タッチ、ディスプレイ) は main.cpp が
// 受け持つ。
//
//   Scene scene;
//   scene.init(width, height);
//   毎フレーム:
//     if (タップされた) scene.onTap();
//     scene.update(seconds);
//     scene.draw(g, 0);          // 画面全体、または
//     scene.draw(g, bandY);      // 画面の一部 (bandY 行目から g のターゲットの高さぶん)

#include <cstdint>

#include "shapoco/gfx2d/graphics2d.hpp"
#include "shapoco/gfx2d/rig.hpp"

namespace meow {

class Scene {
 public:
  // 画面の大きさを与える。kitty はこの大きさから配置される。
  void init(int width, int height);

  // 画面がタップされた: meow モーションを最初から再生し、フキダシを出す
  void onTap();

  // 時刻 t (秒、単調増加) まで進める。毎フレーム draw() の前に 1 回呼ぶ。
  void update(float t);

  // 画面の行 [bandY, bandY + g のターゲットの高さ) を g に描く。
  // g は Graphics2D::init() でアリーナを与えたものを使う。
  void draw(shapoco::gfx2d::Graphics2D &g, int bandY) const;

  // 直近のフレームレート (update() の呼び出し間隔から計算)
  float fps() const { return fps_; }

 private:
  // モーション: 待機 (idle をループ) か、鳴く (meow を 1 回)
  enum class Motion { IDLE, MEOW };

  int width_ = 0, height_ = 0;
  float t_ = 0.0f;

  // kitty の配置 (アーマチュアの原点 = 足元の位置、画面座標)
  int kittyX_ = 0, kittyY_ = 0;

  // rig::Instance::bytes(kitty::armature) は 200 バイト弱
  alignas(4) uint8_t rigMemory_[512];
  shapoco::gfx2d::rig::Instance rig_;

  Motion motion_ = Motion::IDLE;
  float motionStart_ = 0.0f;  // 現在のモーションを始めた時刻
  float bubbleUntil_ = 0.0f;  // この時刻までフキダシを表示する

  // フレームレート (0.5 秒ごとに更新)
  bool started_ = false;
  float fpsT0_ = 0.0f;
  int fpsFrames_ = 0;
  float fps_ = 0.0f;
  char fpsLabel_[24] = "";

  void drawKitty(shapoco::gfx2d::Graphics2D &g, int bandY) const;
  void drawBubble(shapoco::gfx2d::Graphics2D &g, int bandY) const;
  void drawFps(shapoco::gfx2d::Graphics2D &g, int bandY) const;
};

}  // namespace meow

#endif
