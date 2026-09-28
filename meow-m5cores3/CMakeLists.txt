# Meow-M5Cores3: ShapoGFX の 2D リグアニメーションのサンプル (M5Stack CoreS3、ESP-IDF 5.5)。
#
#   ./build.sh [--slow]              # ビルド (ESP-IDF は $IDF_ROOT、既定 ~/esp/5.5)
#   ./run.sh [--slow] [PORT]         # ビルドして書き込み
#   ./monitor.sh [PORT]              # シリアルログ (2 秒ごとにフレームレート)
#
# --slow で低速版 (シングルバッファ + ARGB4444 アトラス)、未指定なら高速版。
# idf.py を直接使うなら -DMEOW_FAST=0 / 1 (CMake のキャッシュに残る)。
#
# ShapoGFX は components/shapogfx/ が $SHAPOGFX_PATH から取り込む。
# M5Unified / M5GFX は main/idf_component.yml に従い、初回ビルド時に
# コンポーネントマネージャが取得する。

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(meow)
