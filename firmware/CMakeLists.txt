cmake_minimum_required(VERSION 3.13)

# プロジェクト名とターゲット
project(UsbSnifferLite C CXX ASM)

# Pico SDK のパスを指定
if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH環境変数が設定されていません")
endif()
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# SDK初期化
pico_sdk_init()

# サブディレクトリ（pico-pio-usb）
add_subdirectory(pico-pio-usb)

# ソースファイル
set(SRC_FILES
    main.c
    utils.c
    capture.c
    display.c
    usb.c
    usb_std.c
    usb_cdc.c
    usb_descriptors.c
    startup_rp2040.c
    binary_info.c
)

# includeディレクトリ
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/pico-pio-usb/src
)

# 実行ファイル生成
add_executable(UsbSnifferLite ${SRC_FILES})

# リンカスクリプト指定
# pico_set_linker_script(UsbSnifferLite ${CMAKE_CURRENT_SOURCE_DIR}/linker/rp2040.ld)

# ライブラリリンク
# pico-pio-usbはadd_subdirectoryでビルドされる
# 必要に応じてtarget_link_librariesに追加

# Pico SDKの標準ライブラリをリンク
pico_enable_stdio_usb(UsbSnifferLite 1)
pico_enable_stdio_uart(UsbSnifferLite 0)
target_link_libraries(UsbSnifferLite pico_stdlib pico_pio_usb)

# UF2ファイル生成
pico_add_extra_outputs(UsbSnifferLite)
