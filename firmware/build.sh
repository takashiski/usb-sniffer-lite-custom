#!/bin/bash
# Raspberry Pi Pico用ファームウェア ビルドスクリプト
# 推奨実行方法: bash build.sh
#
# 必要パッケージ: cmake, make, gcc-arm-none-eabi, build-essential
# 必要環境変数: PICO_SDK_PATH（自動設定可）
#
# エラー時は即終了します
set -e

# --- 設定 ---
FIRMWARE_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$FIRMWARE_DIR/build"
PICO_SDK_PATH_DEFAULT="$HOME/pico-sdk"
TOOLCHAIN_FILE="$PICO_SDK_PATH_DEFAULT/cmake/preload/toolchains/pico_arm_cortex_m0plus_gcc.cmake"

# --- PICO_SDK_PATHの自動設定 ---
if [ -z "$PICO_SDK_PATH" ]; then
  if [ -d "$PICO_SDK_PATH_DEFAULT" ]; then
    export PICO_SDK_PATH="$PICO_SDK_PATH_DEFAULT"
    echo "PICO_SDK_PATHを$PICO_SDK_PATHに自動設定しました。"
  else
    echo "エラー: PICO_SDK_PATHが未設定で、$PICO_SDK_PATH_DEFAULTが見つかりません。"
    echo "pico-sdkを$HOME/pico-sdkにcloneし、PICO_SDK_PATHを設定してください。"
    exit 1
  fi
fi

# --- 必要コマンドの存在確認 ---
for cmd in cmake make arm-none-eabi-gcc; do
  if ! command -v $cmd >/dev/null 2>&1; then
    echo "エラー: $cmd が見つかりません。インストールしてください。"
    exit 1
  fi
done

# --- ビルドディレクトリのクリーン ---
echo "ビルドディレクトリを初期化します: $BUILD_DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# --- CMake実行 ---
echo "CMake構成を開始します..."
cmake .. \
  -DPICO_SDK_PATH="$PICO_SDK_PATH" \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"

# --- make実行 ---
echo "ビルドを開始します..."
make -j$(nproc)

echo "ビルドが完了しました。"
