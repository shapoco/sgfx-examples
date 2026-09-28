# Meow-M5Cores3

[ShapoGFX](https://github.com/shapoco/shapo-gfx) の 2D リグアニメーション (`rig`) を
M5Stack CoreS3 で動かす小さなサンプルです。灰色の画面に子猫 (kitty) が座っていて、
画面をタップすると鳴くモーションを 1 回再生し、1 秒間フキダシで「Meow」と表示します。
左上にはフレームレートが出ます。

- タッチ入力とディスプレイへの転送は M5Unified / M5GFX
- アニメーションとフキダシの描画は ShapoGFX (`Graphics2D` と `rig::Instance`)
- 子猫は `assets/kitty/` の DragonBones データを ShapoGFX の `dbones2cpp` で C++ に
  変換したもの (`main/kitty_*.hpp`、コミット済み)

## ファイル

| ファイル | 内容 |
|---|---|
| `main/main.cpp` | プラットフォーム依存の部分: ボードの立ち上げ、タッチ、フレームバッファの DMA 転送 |
| `main/scene.cpp` / `scene.hpp` | 絵の中身: kitty の姿勢、フキダシ、FPS 表示 (ShapoGFX だけに依存) |
| `main/config.hpp` | 高速版 / 低速版の切り替えマクロ `MEOW_FAST` |
| `main/kitty_fast.hpp` / `kitty_slow.hpp` | `dbones2cpp` の生成物 (`gen_kitty.sh`) |
| `components/shapogfx/` | ShapoGFX を ESP-IDF のコンポーネントとして取り込む |
| `sdkconfig.defaults` | ESP32-S3 の設定 (キャッシュ 64 KB、速度優先の最適化、大きめのアプリ領域) |

## 高速版と低速版

同じ絵を 2 通りの方法で描きます。`MEOW_FAST` マクロで切り替え、既定は高速版です。

| | kitty のデータ | フレームバッファ |
|---|---|---|
| 高速版 (`MEOW_FAST=1`) | 画像ごとのテクスチャ、`--fit-rotate`、RGB565_SWAPPED + キーカラー | 上下 2 分割のダブルバッファ。片方を描く間にもう片方を DMA で転送する |
| 低速版 (`MEOW_FAST=0`) | ARGB4444 のアトラス 1 枚 (既定の設定) | 1 画面ぶんのシングルバッファ。描き終えてから転送する |

内部 SRAM には 320x240 の RGB565 を 2 面 (300 KB) は置けないので、高速版は画面を
上下に分けた 75 KB のバッファを 2 つ使います。PSRAM は使いません。

## 必要なもの

- M5Stack CoreS3
- ESP-IDF 5.5。環境変数 `IDF_ROOT` にインストール先 (`esp-idf/` を含むディレクトリ) を
  指定します。既定は `${HOME}/esp/5.5/` です。
- ShapoGFX のクローン。環境変数 `SHAPOGFX_PATH` に指定します。既定は
  `${HOME}/sgfx/shapo-gfx/` です。

  ```sh
  mkdir -p ${HOME}/sgfx && cd ${HOME}/sgfx
  git clone https://github.com/shapoco/shapo-gfx.git
  export SHAPOGFX_PATH=${HOME}/sgfx/shapo-gfx
  ```

M5Unified と M5GFX は初回ビルド時に ESP-IDF のコンポーネントマネージャが
`main/idf_component.yml` に従って取得します (`managed_components/` に入ります)。

## ビルドと書き込み

```sh
cd meow-m5cores3
./build.sh              # ビルド (高速版)
./build.sh --slow       # 低速版をビルド
./run.sh [PORT]         # ビルドして書き込み (ポート省略時は自動検出)
./run.sh --slow [PORT]  # 低速版をビルドして書き込み
./monitor.sh [PORT]     # シリアルログ (2 秒ごとにフレームレート)
```

`--slow` を付けたときだけ低速版になり、付けなければ常に高速版です (`--fast` でも同じ)。
`idf.py` を直接使う場合は `-DMEOW_FAST=0` / `-DMEOW_FAST=1` を渡します。この指定は
CMake のキャッシュ (`build/`) に残るので、戻すときも明示してください。

## 素材の再生成

`assets/kitty/` を変更したときは `gen_kitty.sh` で `main/kitty_*.hpp` を作り直します。
`dbones2cpp` には Pillow と numpy が要ります。

```sh
python3 -m pip install -r ${SHAPOGFX_PATH}/bin/requirements.txt
./gen_kitty.sh
```

`kitty_ske.json` の先頭には空のアーマチュアがあるため、スクリプトは
`--armature Armature220-kitty-merged` で変換対象を指定しています。
オプションの意味は ShapoGFX のマニュアル (`docsrc/tools/dbones2cpp.rst`) を参照してください。
