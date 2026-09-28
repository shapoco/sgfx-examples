#ifndef MEOW_CONFIG_HPP
#define MEOW_CONFIG_HPP

// 高速版と低速版の切り替え。CMake から MEOW_FAST が渡されなければ高速版。
//
//   MEOW_FAST=1 (既定): kitty_fast.hpp (画像ごとのテクスチャ、RGB565_SWAPPED +
//                      キーカラー) + ダブルバッファ (画面を上下 2 分割し、
//                      片方を描く間にもう片方を DMA で転送する)
//   MEOW_FAST=0:        kitty_slow.hpp (ARGB4444 のアトラス) + シングルバッファ
//                      (1 画面ぶんを描いてから転送する)
//
//   ./build.sh --slow   (idf.py なら -DMEOW_FAST=0 build)
#ifndef MEOW_FAST
#define MEOW_FAST 1
#endif

#endif
