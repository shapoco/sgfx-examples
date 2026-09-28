#!/bin/bash
# ビルドする。IDF_ROOT は ESP-IDF のインストール先 (esp-idf/ を含むディレクトリ)。
#
#   ./build.sh          # 高速版 (既定)
#   ./build.sh --slow   # 低速版
#   ./build.sh --fast   # 高速版 (明示)
#
# 版の指定は CMake のキャッシュに残るので、未指定のときも常に高速版を指定する
# (--slow でビルドした後に引数なしで実行すれば高速版に戻る)。
set -eu
IDF_ROOT="${IDF_ROOT:-${HOME}/esp/5.5}"
cd "$(dirname "$0")"

MEOW_FAST=1
for arg in "$@"; do
  case "$arg" in
    --slow) MEOW_FAST=0 ;;
    --fast) MEOW_FAST=1 ;;
    *) echo "unknown option: $arg (use --slow or --fast)" >&2; exit 1 ;;
  esac
done

source "${IDF_ROOT}/esp-idf/export.sh"
idf.py -DMEOW_FAST=${MEOW_FAST} build
