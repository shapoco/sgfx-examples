#!/bin/bash
# assets/kitty/ のアーマチュアとアニメーションを ShapoGFX の dbones2cpp で
# C++ ヘッダに変換し、main/ に置く。生成物はコミットされているので、素材を
# 変えたときだけ実行すればよい。
#
#   python3 -m pip install -r ${SHAPOGFX_PATH}/bin/requirements.txt   # Pillow, numpy
set -eu
SHAPOGFX_PATH="${SHAPOGFX_PATH:-${HOME}/sgfx/shapo-gfx}"
cd "$(dirname "$0")"

DBONES2CPP="${SHAPOGFX_PATH}/bin/dbones2cpp"
ASSETS=../assets/kitty
COMMON_OPTS=(--scale 0.5)
# idle と meow のアニメーションは kitty_ske.json に含まれている
INPUTS=("${ASSETS}/kitty_ske.json")

# 低速版: ARGB4444 のアトラス 1 枚 (既定の設定)
python3 "${DBONES2CPP}" "${COMMON_OPTS[@]}" --namespace kitty_slow \
    "${INPUTS[@]}" main/kitty_slow.hpp

# 高速版: 画像ごとのテクスチャ、余白を落とす回転、フレームバッファと同じ
# RGB565_SWAPPED (透明はキーカラー)
python3 "${DBONES2CPP}" "${COMMON_OPTS[@]}" --namespace kitty_fast \
    --atlas-width 0 --fit-rotate --out-format rgb565_swapped \
    "${INPUTS[@]}" main/kitty_fast.hpp
