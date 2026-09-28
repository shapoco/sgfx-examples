// Meow-M5Cores3: M5Stack CoreS3 で動く ShapoGFX の 2D リグアニメーションのサンプル。
//
// このファイルはプラットフォームに依存する部分: M5Unified でボードを立ち上げ、
// タッチを読み、ShapoGFX が描いたフレームバッファを M5GFX で SPI DMA 転送する。
// 絵の中身は scene.cpp にある。
//
// フレームバッファは内部 SRAM に置く (PSRAM は使わない)。1 画面 (320x240 の
// RGB565) は 150 KB で、2 画面ぶんは持てないので、高速版は画面を上下に分けた
// 半分ずつのバッファを 2 つ持ち、片方を描く間にもう片方を転送する。

#include <M5Unified.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdio>
#include <cstring>

#include "config.hpp"
#include "scene.hpp"
#include "shapoco/gfx2d/graphics2d.hpp"

namespace g2 = shapoco::gfx2d;

namespace {

// ディスプレイの向き。CoreS3 では 1 が横長 (320x240)。
constexpr int DISPLAY_ROTATION = 1;
constexpr int DISPLAY_BRIGHTNESS = 200;

// フレームバッファのピクセル形式。M5GFX がパネルに送るバイト順そのままなので、
// writePixelsDMA(..., swap = false) にバッファを直接渡せる。
constexpr g2::PixelFormat FRAME_FORMAT = g2::PixelFormat::RGB565_SWAPPED;
constexpr int BYTES_PER_PIXEL = 2;

// 画面を縦にいくつの帯に分けて描くか
#if MEOW_FAST
constexpr int BAND_COUNT = 2;   // 上下 2 分割、バッファ 2 面
constexpr int BUFFER_COUNT = 2;
#else
constexpr int BAND_COUNT = 1;   // 画面全体、バッファ 1 面
constexpr int BUFFER_COUNT = 1;
#endif

// Graphics2D の作業メモリ (状態スタックと多角形の一時領域)
constexpr size_t ARENA_BYTES = 4096;

// シリアルにフレームレートを出す間隔
constexpr int64_t LOG_INTERVAL_US = 2 * 1000 * 1000;

meow::Scene g_scene;
g2::Graphics2D g_gfx;
alignas(8) uint8_t g_arena[ARENA_BYTES];

int g_width = 0, g_height = 0, g_bandHeight = 0;
uint16_t *g_buffers[BUFFER_COUNT] = {};

// 帯のバッファを内部 SRAM に確保する。DMA が読むので MALLOC_CAP_DMA。
bool allocateBuffers() {
  const size_t bytes = (size_t)g_width * g_bandHeight * BYTES_PER_PIXEL;
  for (int i = 0; i < BUFFER_COUNT; i++) {
    g_buffers[i] = (uint16_t *)heap_caps_malloc(
        bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!g_buffers[i]) {
      std::printf("frame buffer %d (%u bytes) did not fit, largest block %u\n",
                  i, (unsigned)bytes,
                  (unsigned)heap_caps_get_largest_free_block(
                      MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
      return false;
    }
    std::memset(g_buffers[i], 0, bytes);
  }
  return true;
}

// 帯 (画面の行 [y, y + rows)) をバッファに描く
void drawBand(uint16_t *pixels, int y, int rows) {
  g_gfx.setTarget({FRAME_FORMAT, (int16_t)g_width, (int16_t)rows,
                   (uint32_t)(g_width * BYTES_PER_PIXEL), pixels});
  g_scene.draw(g_gfx, y);
}

// 帯をパネルへ送り始める。すぐに戻り、転送は DMA が続ける。
void sendBand(const uint16_t *pixels, int y, int rows) {
  // setWindow の終点は含む座標
  M5.Display.setWindow(0, y, g_width - 1, y + rows - 1);
  M5.Display.writePixelsDMA(pixels, (int32_t)g_width * rows, false);
}

// 画面がタップされた瞬間 (押し始め) かどうか
bool tapped() {
  if (M5.Touch.getCount() == 0) return false;
  return M5.Touch.getDetail(0).wasPressed();
}

void haltForever() {
  for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}

}  // namespace

extern "C" void app_main(void) {
  // 使わない内蔵デバイスは初期化しない
  auto cfg = M5.config();
  cfg.internal_spk = false;
  cfg.internal_mic = false;
  cfg.internal_imu = false;
  cfg.internal_rtc = false;
  M5.begin(cfg);
  M5.Display.setRotation(DISPLAY_ROTATION);
  M5.Display.setBrightness(DISPLAY_BRIGHTNESS);
  M5.Display.fillScreen(TFT_BLACK);

  // 画面の大きさはディスプレイから取る
  g_width = M5.Display.width();
  g_height = M5.Display.height();
  g_bandHeight = (g_height + BAND_COUNT - 1) / BAND_COUNT;
  std::printf("meow: %dx%d, %s, %d band(s) of %d rows\n", g_width, g_height,
              MEOW_FAST ? "fast (double buffer)" : "slow (single buffer)",
              BAND_COUNT, g_bandHeight);

  if (!allocateBuffers()) haltForever();
  g_gfx.init(g_arena, ARENA_BYTES);
  g_scene.init(g_width, g_height);

  // SPI のトランザクションは開いたままにする。他にバスを使うものはなく、
  // endWrite() は転送の完了を待ってしまう。
  M5.Display.startWrite();

  std::printf("internal RAM free %u, largest block %u\n",
              (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
              (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

  const int64_t start = esp_timer_get_time();
  int64_t lastLog = start;
  for (;;) {
    const int64_t now = esp_timer_get_time();
    M5.update();
    if (tapped()) g_scene.onTap();
    g_scene.update((float)(now - start) * 1e-6f);

    // 帯を順に描いて送る。帯 i はバッファ i % BUFFER_COUNT を使う。
    //
    // 高速版 (帯 2、バッファ 2): 帯 0 を描く間に前のフレームの帯 1 が転送され、
    // 帯 1 を描く間に帯 0 が転送される。描く前に waitDMA() は要らない: その
    // バッファの転送は、直前の帯を送る前の waitDMA() で終わっている。
    //
    // 低速版 (帯 1、バッファ 1): 描いてから送り、終わるのを待つ。
    for (int band = 0; band < BAND_COUNT; band++) {
      const int y = band * g_bandHeight;
      const int rows = (g_height - y < g_bandHeight) ? g_height - y : g_bandHeight;
      uint16_t *buffer = g_buffers[band % BUFFER_COUNT];
      drawBand(buffer, y, rows);
      M5.Display.waitDMA();  // 前の帯の転送が終わるのを待つ
      sendBand(buffer, y, rows);
    }
#if !MEOW_FAST
    // バッファが 1 つしかないので、次に描く前に転送を終わらせる
    M5.Display.waitDMA();
#endif

    if (now - lastLog >= LOG_INTERVAL_US) {
      lastLog = now;
      const int fps10 = (int)(g_scene.fps() * 10 + 0.5f);
      std::printf("%d.%d fps\n", fps10 / 10, fps10 % 10);
    }
  }
}
