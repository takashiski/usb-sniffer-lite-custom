# 環境構築方法

このドキュメントでは、RP2040用USBスニファーの開発・ビルド環境の構築手順をまとめます。

## 前提条件
- OS: Linux（bashシェル推奨）
- Raspberry Pi Pico（RP2040）
- 必要なツール: git, cmake, make, arm-none-eabi-gcc, Python3

## 1. リポジトリのクローン

```bash
git clone --recursive <このリポジトリのURL>
cd usb-sniffer-lite-custom
```

※ `--recursive` オプションでサブモジュールも同時に取得します。

## 2. 必要なパッケージのインストール

Ubuntuの場合:
```bash
sudo apt update
sudo apt install cmake make gcc-arm-none-eabi build-essential python3 git
```

## 3. ファームウェアのビルド

```bash
cd firmware
mkdir -p build
cd build
cmake ..
make
```

- ビルドが成功すると `usb_sniffer.uf2` などのファイルが生成されます。

## 4. ファームウェアの書き込み

1. Raspberry Pi PicoをBOOTSELボタンを押しながらPCに接続し、USBドライブとしてマウントします。
2. `bin/UsbSnifferLite.uf2` または `firmware/build/` 内の `.uf2` ファイルをドラッグ＆ドロップでコピーします。

## 5. ハードウェア接続

- USB D+（緑）: GPIO10
- USB D-（白）: GPIO11
- GND（黒）: GND
- その他のピン割り当てや詳細は `doc/Hardware.md` を参照してください。

## 6. 動作確認

- シリアルターミナル（例: minicom, screen, Tera Term等）でRP2040のVCP（仮想COMポート）に接続し、コマンド送信やデータ取得が可能です。

## 参考
- 詳細な使い方やコマンド一覧は `README.md` を参照してください。
- サブモジュール `firmware/pico-pio-usb` の詳細は同ディレクトリの `README.md` も参照。
