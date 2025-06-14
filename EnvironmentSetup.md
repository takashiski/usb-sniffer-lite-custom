# 環境構築手順（WSL2 Ubuntu 24.04）

このプロジェクトをWSL2上のUbuntu 24.04でビルドするための手順をまとめます。

## 1. 必要なパッケージのインストール

以下のコマンドで必要なパッケージをインストールしてください。

```bash
sudo apt update
sudo apt install -y gcc-arm-none-eabi make cmake git
```

## 2. サブモジュールの初期化

`pico-pio-usb` ディレクトリはサブモジュールです。以下のコマンドで初期化・更新してください。

```bash
git submodule update --init --recursive
```

## 3. ファームウェアのビルド

以下のコマンドでファームウェアをビルドできます。

```bash
cd firmware
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

または、ビルドスクリプトを使う場合は以下のコマンドでもOKです。

```bash
cd firmware
bash build.sh clean
```

ビルドが成功すると、`UsbSnifferLite.elf.uf2` などのファイルが `firmware/build` ディレクトリに生成されます。

## 4. Raspberry Pi Pico SDK のセットアップ

一部の開発環境や拡張機能によっては Raspberry Pi Pico SDK のセットアップが必要になる場合があります。以下の手順でインストールできます。

```bash
git clone --depth=1 https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
```

SDKのパスを環境変数に設定する場合は、以下のコマンドを実行してください（必要に応じて .bashrc などに追記してください）。

```bash
export PICO_SDK_PATH=~/pico-sdk
```

## 5. 補足
- その他詳細は `README.md` も参照してください。

---
何か問題が発生した場合は、エラーメッセージとともにご相談ください。
