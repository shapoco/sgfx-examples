// Meow のシーン (scene.hpp)。

#include "scene.hpp"

#include <cstdio>

#include "config.hpp"
#include "shapoco/gfx2d/fonts.hpp"

// dbones2cpp が assets/kitty/ から生成したデータ (gen_kitty.sh)。
// 高速版と低速版は名前空間が違うだけで、中身の構成は同じ。
#if MEOW_FAST
#include "kitty_fast.hpp"
namespace kitty = kitty_fast;
#else
#include "kitty_slow.hpp"
namespace kitty = kitty_slow;
#endif

namespace meow {

namespace g2 = shapoco::gfx2d;
namespace rig = shapoco::gfx2d::rig;

namespace {

// --- 配置 -------------------------------------------------------------------

// kitty の足元を置く位置: 画面幅のこの割合の x、y は画面の下端
constexpr float KITTY_X_RATIO = 2.0f / 3.0f;

// フキダシの先端 (三角形の頂点) の位置: kitty の外接矩形の左上からの距離
constexpr int BUBBLE_TIP_DX = 28;
constexpr int BUBBLE_TIP_DY = 30;
// 先端からフキダシ本体 (角丸矩形) の右下までの距離
constexpr int BUBBLE_TAIL_DX = 8;
constexpr int BUBBLE_TAIL_DY = 10;
// 三角形の底辺: 本体の下辺の右端から左へこの幅
constexpr int BUBBLE_TAIL_WIDTH = 16;
constexpr int BUBBLE_TAIL_INSET = 12;  // 底辺の右端を本体の右端から内側へ
constexpr int BUBBLE_PADDING_X = 12;   // 文字の周りの余白
constexpr int BUBBLE_PADDING_Y = 6;
constexpr int BUBBLE_RADIUS = 8;

// FPS 表示の位置 (左上隅からの距離)
constexpr int FPS_X = 4;
constexpr int FPS_Y = 4;

// --- 時間 -------------------------------------------------------------------

constexpr float BUBBLE_SECONDS = 1.0f;       // フキダシを出す時間
constexpr float FPS_INTERVAL_SECONDS = 0.5f;  // フレームレートを更新する間隔

// --- 色 -------------------------------------------------------------------

constexpr g2::Color BACKGROUND_COLOR = g2::makeColor(128, 128, 128);
constexpr g2::Color BUBBLE_COLOR = g2::Colors::WHITE;
constexpr g2::Color BUBBLE_TEXT_COLOR = g2::makeColor(255, 96, 160);  // ピンク
constexpr g2::Color FPS_COLOR = g2::Colors::WHITE;

const char *const BUBBLE_TEXT = "Meow";
const GFXfont &BUBBLE_FONT = ShapoSansP_s21c16a01w03;
const GFXfont &FPS_FONT = ShapoSansP_s12c09a01w02;

// アニメーションの長さ (秒)
float animationSeconds(const rig::Animation &anim) {
  return (float)anim.duration / (float)anim.frameRate;
}

}  // namespace

void Scene::init(int width, int height) {
  width_ = width;
  height_ = height;
  kittyX_ = (int)(width * KITTY_X_RATIO);
  kittyY_ = height;
  rig_.init(kitty::armature, rigMemory_, sizeof(rigMemory_));
  update(0.0f);
}

void Scene::onTap() {
  motion_ = Motion::MEOW;
  motionStart_ = t_;
  bubbleUntil_ = t_ + BUBBLE_SECONDS;
}

void Scene::update(float t) {
  t_ = t;

  // フレームレート: 一定時間ごとにフレーム数を数え直す
  if (!started_) {
    started_ = true;
    fpsT0_ = t;
  }
  fpsFrames_++;
  if (t - fpsT0_ >= FPS_INTERVAL_SECONDS) {
    fps_ = fpsFrames_ / (t - fpsT0_);
    fpsFrames_ = 0;
    fpsT0_ = t;
    // printf の浮動小数点は使わず、10 倍した整数で小数 1 桁を作る
    const int fps10 = (int)(fps_ * 10 + 0.5f);
    std::snprintf(fpsLabel_, sizeof(fpsLabel_), "%d.%d fps", fps10 / 10,
                  fps10 % 10);
  }

  // meow を最後まで再生したら待機に戻る。meow の最後の姿勢は idle の最初の
  // 姿勢と同じなので、idle をフレーム 0 から始めればつながって見える。
  if (motion_ == Motion::MEOW &&
      t - motionStart_ >= animationSeconds(kitty::anim_meow)) {
    motion_ = Motion::IDLE;
    motionStart_ = t;
  }

  // 姿勢を更新する。データは 24 fps だが、小数フレームは補間される。
  const float elapsed = t - motionStart_;
  if (motion_ == Motion::MEOW) {
    const rig::Animation &anim = kitty::anim_meow;
    rig_.pose(anim, rig::frameAt(anim, elapsed, /* loop = */ false));
  } else {
    const rig::Animation &anim = kitty::anim_idle;
    rig_.pose(anim, rig::frameAt(anim, elapsed, /* loop = */ true));
  }
}

void Scene::draw(g2::Graphics2D &g, int bandY) const {
  g.resetClipRect();
  g.clear(BACKGROUND_COLOR);
  drawKitty(g, bandY);
  if (t_ < bubbleUntil_) drawBubble(g, bandY);
  drawFps(g, bandY);
}

// 画面座標 (x, y) を、bandY 行目から始まる帯の座標にする変換
static g2::affine2f screenToBand(int x, int y, int bandY) {
  return g2::affine2f::translation((float)x, (float)(y - bandY));
}

void Scene::drawKitty(g2::Graphics2D &g, int bandY) const {
  // 変換行列がアーマチュアの配置になる。帯の外にあるパーツは描かれない。
  g.setTransform(screenToBand(kittyX_, kittyY_, bandY));
  rig_.draw(g);
}

void Scene::drawBubble(g2::Graphics2D &g, int bandY) const {
  g.setTransform(screenToBand(0, 0, bandY));

  // フキダシの大きさは文字の大きさから決める
  g.setFont(&BUBBLE_FONT);
  const g2::TextMetrics tm = g.textMetrics(BUBBLE_TEXT);
  const int bodyW = tm.width + BUBBLE_PADDING_X * 2;
  const int bodyH = tm.height + BUBBLE_PADDING_Y * 2;

  // 先端は kitty の外接矩形 (バインドポーズ) の左上のあたり、本体はその左上
  const g2::RectF &kb = kitty::armature.bounds;
  const int tipX = kittyX_ + (int)kb.x + BUBBLE_TIP_DX;
  const int tipY = kittyY_ + (int)kb.y + BUBBLE_TIP_DY;
  const int bodyX = tipX - BUBBLE_TAIL_DX - bodyW;
  const int bodyY = tipY - BUBBLE_TAIL_DY - bodyH;

  // 本体 (角丸矩形) と、先端へ伸びる三角形。同じ色なので継ぎ目は見えない。
  g.fillRoundRect(bodyX, bodyY, bodyW, bodyH, BUBBLE_RADIUS, BUBBLE_COLOR);
  const int baseRight = bodyX + bodyW - BUBBLE_TAIL_INSET;
  const int baseY = bodyY + bodyH - 1;
  const g2::vec2i tail[3] = {
      {baseRight - BUBBLE_TAIL_WIDTH, baseY},
      {baseRight, baseY},
      {tipX, tipY},
  };
  g.fillPolygon(tail, 3, BUBBLE_COLOR);

  g.setTextColor(BUBBLE_TEXT_COLOR);
  g.drawString(bodyX + BUBBLE_PADDING_X, bodyY + BUBBLE_PADDING_Y, BUBBLE_TEXT);
}

void Scene::drawFps(g2::Graphics2D &g, int bandY) const {
  g.setTransform(screenToBand(0, 0, bandY));
  g.setFont(&FPS_FONT);
  g.setTextColor(FPS_COLOR);
  g.drawString(FPS_X, FPS_Y, fpsLabel_);
}

}  // namespace meow
