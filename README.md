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

## クライアント側での注意点・COMポート設定

- 仮想COMポートは「USBシリアルデバイス」として認識されます。
- **推奨設定：ボーレート115200、データビット8、パリティなし（None）、ストップビット1、フロー制御なし（None）**
    - 例: 115200 8N1（115200bps, 8bit, No parity, 1 stop bit）
    - これ以外の値でも通信自体は可能ですが、端末ソフトによっては設定が必要な場合があります。
- フロー制御（RTS/CTS, XON/XOFF）は無効にしてください。
- 端末ソフト（Tera Term, minicom, screen, PuTTY等）で「改行コード=LF」推奨。
- 受信バッファが小さい場合、データロストすることがあります。できるだけ大きめのバッファや高速なログ保存を推奨します。
- Windowsの場合、ドライバ不要ですが、デバイス名は「USB Serial Device」等になります。
- Linuxの場合、/dev/ttyACM0 などとして認識されます。
- macOSの場合、/dev/tty.usbmodem* などとして認識されます。

## ハードウェア接続例
| RP2040 Pin | Function | USB Cable Color |
|:-------:|:----------------:|:-----:|
| GND     | Ground           | Black |
| GPIO 10 | D+               | Green |
| GPIO 11 | D-               | White |

## 参考
- オンラインUSBパケットデコーダ: https://eleccelerator.com/usbdescreqparser/
