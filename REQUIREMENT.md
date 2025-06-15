# プロジェクト要件: RP2040用USBスニファー（日本語）

## 目的

- Nitendo Switch proコントローラーのUSB信号を盗聴し、レポートディスクリプタごとバイト列をCOMポート経由でPCに送信するプログラムを作成すること
    - PCではUSB信号をパースしてボタン入力やスティック操作・ジャイロ操作として読み取り、画面に表示する（本プロジェクト対象外）

## 要件

- raspberry pi pico(RP2040)を使うこと
- USB D+/D-信号はそれぞれGPIO10: DM, GPIO11: DPに接続する

## 参考

* https://github.com/ataradov/usb-sniffer-lite
* https://github.com/retrospy/usb-sniffer-lite
* https://chromium.googlesource.com/chromium/src/+/HEAD/device/gamepad/nintendo_controller.cc
* https://github.com/therealdreg/pico-usb-sniffer-lite/

