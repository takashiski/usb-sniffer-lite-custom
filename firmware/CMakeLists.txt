cmake_minimum_required(VERSION 3.13)

# プロジェクト名とターゲット
project(usb_sniffer_pico C CXX ASM)

# Pico SDK のパスを指定
if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH環境変数が設定されていません")
endif()
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# SDK初期化
pico_sdk_init()

# ソースファイル
set(SRC_FILES
    main.c
    usb_sniffer.pio
)

# 実行ファイル生成
add_executable(usb_sniffer_pico ${SRC_FILES})
pico_generate_pio_header(usb_sniffer_pico ${CMAKE_CURRENT_LIST_DIR}/usb_sniffer.pio)
target_link_libraries(usb_sniffer_pico pico_stdlib hardware_pio hardware_irq tinyusb_device)
pico_add_extra_outputs(usb_sniffer_pico)
