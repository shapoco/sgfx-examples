# ShapoGFX を ESP-IDF のコンポーネントとして取り込む (gfx2d のみ)。
# 場所は環境変数 SHAPOGFX_PATH、未設定なら ${HOME}/sgfx/shapo-gfx。
if(DEFINED ENV{SHAPOGFX_PATH})
    set(SHAPOGFX_ROOT $ENV{SHAPOGFX_PATH})
else()
    set(SHAPOGFX_ROOT $ENV{HOME}/sgfx/shapo-gfx)
endif()
if(NOT EXISTS ${SHAPOGFX_ROOT}/include/shapoco/gfx2d/graphics2d.hpp)
    message(FATAL_ERROR
        "ShapoGFX not found at ${SHAPOGFX_ROOT}: set SHAPOGFX_PATH to the clone of https://github.com/shapoco/shapo-gfx")
endif()

idf_component_register(
    SRCS
        "${SHAPOGFX_ROOT}/src/gfx2d/graphics2d.cpp"
        "${SHAPOGFX_ROOT}/src/gfx2d/images.cpp"
        "${SHAPOGFX_ROOT}/src/gfx2d/rig.cpp"
        "${SHAPOGFX_ROOT}/src/gfx2d/shapes.cpp"
    INCLUDE_DIRS "${SHAPOGFX_ROOT}/include"
)

# このサンプルが使うのは ARGB4444 (低速版のアトラス) と RGB565_SWAPPED だけなので、
# GRAY1 と RGB444 はコンパイルしない。ヘッダも参照するマクロなので、
# ターゲット単位ではなくビルド全体に設定する。
idf_build_set_property(COMPILE_DEFINITIONS "SHAPOGFX_FORMAT_GRAY1=0" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "SHAPOGFX_FORMAT_RGB444=0" APPEND)
