#!/bin/bash
# シリアルログを表示する (2 秒ごとにフレームレート)。
set -eu
IDF_ROOT="${IDF_ROOT:-${HOME}/esp/5.5}"
cd "$(dirname "$0")"
source "${IDF_ROOT}/esp-idf/export.sh"
if [ $# -ge 1 ]; then idf.py -p "$1" monitor; else idf.py monitor; fi
