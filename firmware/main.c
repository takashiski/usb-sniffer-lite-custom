// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "usb_sniffer.pio.h"
#include "tusb.h"
#include "pico/binary_info.h"

/*- Definitions -------------------------------------------------------------*/
#define USB_DMINUS_PIN 10
#define USB_DPLUS_PIN  11
#define CDC_BUF_SIZE   64
#define USB_SAMPLE_BUF_SIZE 128

/*- Variables ---------------------------------------------------------------*/
volatile bool sniffing = false;

// NRZIデコード用の状態
static uint8_t prev_nrzi = 0;
static uint8_t bit_buf = 0;
static int bit_count = 0;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static int nrzi_decode(uint8_t dplus, uint8_t dminus) {
    uint8_t curr = (dplus << 1) | dminus;
    int bit = (curr != prev_nrzi) ? 1 : 0;
    prev_nrzi = curr;
    return bit;
}

// サンプルバッファからバイト列を生成しCDC送信
static void process_usb_samples(uint8_t *samples, int sample_len) {
    static uint8_t cdc_buf[CDC_BUF_SIZE];
    static int cdc_ptr = 0;
    for (int i = 0; i < sample_len; i++) {
        uint8_t d = samples[i];
        int bit = nrzi_decode((d >> 1) & 1, d & 1);
        bit_buf = (bit_buf >> 1) | (bit ? 0x80 : 0x00); // LSBファースト
        bit_count++;
        if (bit_count == 8) {
            cdc_buf[cdc_ptr++] = bit_buf;
            bit_count = 0;
            bit_buf = 0;
            if (cdc_ptr >= CDC_BUF_SIZE) {
                tud_cdc_write(cdc_buf, CDC_BUF_SIZE);
                tud_cdc_write_flush();
                cdc_ptr = 0;
            }
        }
    }
}

// 起動時メッセージとコマンド一覧
void print_startup_message(void) {
    tud_cdc_write_str("[UsbSnifferLite] 起動しました\r\n");
    tud_cdc_write_str("コマンド一覧:\r\n");
    tud_cdc_write_str("  s: キャプチャ開始\r\n");
    tud_cdc_write_str("  t: キャプチャ停止\r\n");
    tud_cdc_write_str("\r\n");
    tud_cdc_write_flush();
}

// TinyUSB CDC受信コールバック
void tud_cdc_rx_cb(uint8_t itf) {
    char buf[8];
    uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
    for (uint32_t i = 0; i < count; i++) {
        if (buf[i] == 's') {
            // キャプチャ開始
            tud_cdc_write_str("キャプチャ開始\r\n");
            tud_cdc_write_flush();
            sniffing = true;
        } else if (buf[i] == 't') {
            // キャプチャ停止
            tud_cdc_write_str("キャプチャ停止\r\n");
            tud_cdc_write_flush();
            sniffing = false;
        }
    }
}

// TinyUSB標準のデバイスディスクリプタ定義
#include "tusb.h"

static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4000,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// CDCのみの最小構成
#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, CONFIG_TOTAL_LEN, 0, 100),
    TUD_CDC_DESCRIPTOR(0, 4, 0x81, 8, 0x02, 0x82, 64)
};

static const char* desc_string[] = {
    "",
    "UsbSnifferLite", // Manufacturer
    "RP2040 USB Sniffer", // Product
    "123456", // Serial
    "CDC Interface" // CDC
};

// コールバック実装
uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    static uint16_t desc_str[32];
    uint8_t chr_count;
    (void)langid;
    if (index == 0) {
        desc_str[1] = 0x0409;
        chr_count = 1;
    } else {
        const char *str = (index < sizeof(desc_string)/sizeof(desc_string[0])) ? desc_string[index] : NULL;
        if (!str) return NULL;
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) {
            desc_str[1 + i] = str[i];
        }
    }
    desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return desc_str;
}

int main() {
    stdio_init_all();
    tusb_init();
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &usb_sniffer_program);
    pio_sm_config c = usb_sniffer_program_get_default_config(offset);
    pio_sm_set_consecutive_pindirs(pio, sm, USB_DMINUS_PIN, 2, false);
    pio_sm_set_in_pins(pio, sm, USB_DMINUS_PIN);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    uint8_t sample_buf[USB_SAMPLE_BUF_SIZE];
    int sample_ptr = 0;

    print_startup_message();

    while (true) {
        tud_task();
        if (sniffing) {
            while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
                uint32_t sample = pio_sm_get(pio, sm) & 0x03; // D+/D-の2bit
                sample_buf[sample_ptr++] = (uint8_t)sample;
                if (sample_ptr >= USB_SAMPLE_BUF_SIZE) {
                    process_usb_samples(sample_buf, sample_ptr);
                    sample_ptr = 0;
                }
            }
        } else {
            sample_ptr = 0; // 停止時はバッファをクリア
            bit_count = 0;
            bit_buf = 0;
        }
    }
    return 0;
}

// プログラム名と説明をバイナリ情報として埋め込む
bi_decl(bi_program_name("UsbSnifferLite"));
bi_decl(bi_program_description("USB Sniffer Lite Custom Firmware"));
