# Usb Sniffer Lite for RP2040

Raspberry Pi Pico (RP2040) を使ったシンプルなUSBスニファーです。

## ビルド方法

### 必要なもの
- cmake, make, gcc-arm-none-eabi, build-essential
- Raspberry Pi Pico SDK (PICO_SDK_PATH)

### CMakeビルド手順（推奨）
1. Pico SDKを $HOME/pico-sdk にclone（またはPICO_SDK_PATHを適切に設定）
2. firmware ディレクトリで `bash build.sh` を実行
3. `build/UsbSnifferLite.elf.uf2` などが生成されます

### Makefileビルド手順（サブディレクトリmake/）
1. firmware/make ディレクトリで `make` を実行
2. `make/build/UsbSnifferLite.uf2` などが生成されます

### ファームウェア書き込み
PicoをBOOTSELモードでPCに接続し、生成されたUF2ファイルをドラッグ＆ドロップしてください。

## 使い方
- USBケーブルのD+/D-をPicoのGPIO10/11に接続
- PC側で仮想COMポートとして認識されます
- "START"コマンドでキャプチャ開始、"STOP"で停止
- 取得データはバイト列としてCOMポートに出力されます

## ハードウェア接続例
| RP2040 Pin | Function | USB Cable Color |
|:-------:|:----------------:|:-----:|
| GND     | Ground           | Black |
| GPIO 10 | D+               | Green |
| GPIO 11 | D-               | White |

## 参考
- オンラインUSBパケットデコーダ: https://eleccelerator.com/usbdescreqparser/
