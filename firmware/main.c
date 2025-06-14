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

//-----------------------------------------------------------------------------
void on_cdc_rx(void) {
    uint8_t buf[64];
    uint32_t count = tud_cdc_n_read(0, buf, sizeof(buf));
    if (count > 0) {
        if (count >= 5 && !memcmp(buf, "START", 5)) sniffing = true;
        if (count >= 4 && !memcmp(buf, "STOP", 4)) sniffing = false;
    }
}

//-----------------------------------------------------------------------------
int main() {
    stdio_init_all();
    tusb_init();
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &usb_sniffer_program);
    usb_sniffer_program_init(pio, sm, offset, USB_DMINUS_PIN, USB_DPLUS_PIN);
    tud_cdc_set_rx_cb(on_cdc_rx);

    uint8_t sample_buf[USB_SAMPLE_BUF_SIZE];
    int sample_ptr = 0;

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
