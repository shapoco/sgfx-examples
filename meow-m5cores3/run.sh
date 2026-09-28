#!/bin/bash
# ビルドして書き込む。
#
#   ./run.sh [--slow|--fast] [PORT]
#
# --slow で低速版、未指定または --fast で高速版 (build.sh と同じ)。
# PORT を省略すると esptool が自動検出する。
set -eu
IDF_ROOT="${IDF_ROOT:-${HOME}/esp/5.5}"
cd "$(dirname "$0")"

MEOW_FAST=1
PORT=""
for arg in "$@"; do
  case "$arg" in
    --slow) MEOW_FAST=0 ;;
    --fast) MEOW_FAST=1 ;;
    -*) echo "unknown option: $arg (use --slow or --fast)" >&2; exit 1 ;;
    *) PORT="$arg" ;;
  esac
done

source "${IDF_ROOT}/esp-idf/export.sh"
idf.py -DMEOW_FAST=${MEOW_FAST} build
if [ -n "$PORT" ]; then idf.py -p "$PORT" flash; else idf.py flash; fi
