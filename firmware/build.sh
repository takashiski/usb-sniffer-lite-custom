#!/bin/bash
# Raspberry Pi Pico用ファームウェア CMakeビルドスクリプト
# 推奨実行方法: bash build.sh
#
# 必要パッケージ: cmake, make, gcc-arm-none-eabi, build-essential
# 必要環境変数: PICO_SDK_PATH（自動設定可）
set -e

# --- 設定 ---
FIRMWARE_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$FIRMWARE_DIR/build"
PICO_SDK_PATH_DEFAULT="$HOME/pico-sdk"

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
CLEAN_BUILD=0
for arg in "$@"; do
  if [ "$arg" = "clean" ]; then
    CLEAN_BUILD=1
  fi
  if [ "$arg" = "help" ] || [ "$arg" = "--help" ] || [ "$arg" = "-h" ]; then
    echo "Usage: bash build.sh [clean]"
    echo "  clean : ビルドディレクトリを初期化してクリーンビルド"
    exit 0
  fi
done

if [ $CLEAN_BUILD -eq 1 ]; then
  echo "ビルドディレクトリを初期化します..."
  rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DPICO_SDK_PATH="$PICO_SDK_PATH" ..
make -j$(nproc)

cd "$FIRMWARE_DIR"
echo "ビルド完了: build/UsbSnifferLite.elf.uf2 などを確認してください。"
