#!/bin/bash
set -e

# Makefileベースでビルド
cd "$(dirname "$0")/make"
make clean
make

echo "ビルドが完了しました。firmware/make/build ディレクトリを確認してください。"
