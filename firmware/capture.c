// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "rp2040.h"
#include "hal_gpio.h"
#include "pio_asm.h"
#include "capture.h"
#include "display.h"
#include "globals.h"
#include "../pico-pio-usb/src/pio_usb.h"

/*- Definitions -------------------------------------------------------------*/
#define CORE1_STACK_SIZE       512 // words

// DP and DM can be any pins, but they must be consequitive and in that order
#define DP_INDEX       10
#define DM_INDEX       11
#define START_INDEX    12

HAL_GPIO_PIN(DP,       0, 10, pio0_10)
HAL_GPIO_PIN(DM,       0, 11, pio0_11)
HAL_GPIO_PIN(START,    0, 12, pio1_12) // Internal trigger from PIO1 to PIO0
HAL_GPIO_PIN(TRIGGER,  0, 18, sio_18)

/*- Constants ---------------------------------------------------------------*/

/*- Variables ---------------------------------------------------------------*/
uint32_t g_buffer[BUFFER_SIZE];
buffer_info_t g_buffer_info;

int g_capture_speed   = CaptureSpeed_Full;
int g_capture_trigger = CaptureTrigger_Disabled;
int g_capture_limit   = CaptureLimit_Unlimited;
int g_display_time    = DisplayTime_SOF;
int g_display_data    = DisplayData_Full;
int g_display_fold    = DisplayFold_Enabled;

int g_capture_stream_mode = 0; // 0:バッファ一括, 1:逐次送信

void capture_set_stream_mode(int mode) { g_capture_stream_mode = mode; }

static const char *capture_speed_str[CaptureSpeedCount] = {
  [CaptureSpeed_Low]  = "Low",
  [CaptureSpeed_Full] = "Full",
};
static const char *capture_trigger_str[CaptureTriggerCount] = {
  [CaptureTrigger_Enabled]  = "Enabled",
  [CaptureTrigger_Disabled] = "Disabled",
};
static const char *capture_limit_str[CaptureLimitCount] = {
  [CaptureLimit_100]       = "100 packets",
  [CaptureLimit_200]       = "200 packets",
  [CaptureLimit_500]       = "500 packets",
  [CaptureLimit_1000]      = "1000 packets",
  [CaptureLimit_2000]      = "2000 packets",
  [CaptureLimit_5000]      = "5000 packets",
  [CaptureLimit_10000]     = "10000 packets",
  [CaptureLimit_Unlimited] = "Unlimited",
};
static const char *display_time_str[DisplayTimeCount] = {
  [DisplayTime_First]    = "Relative to the first packet",
  [DisplayTime_Previous] = "Relative to the previous packet",
  [DisplayTime_SOF]      = "Relative to the SOF",
  [DisplayTime_Reset]    = "Relative to the bus reset",
};
static const char *display_data_str[DisplayDataCount] = {
  [DisplayData_Full]    = "Full",
  [DisplayData_Limit16] = "Limit to 16 bytes",
  [DisplayData_Limit64] = "Limit to 64 bytes",
  [DisplayData_None]    = "Do not display data",
};
static const char *display_fold_str[DisplayFoldCount] = {
  [DisplayFold_Enabled]  = "Enabled",
  [DisplayFold_Disabled] = "Disabled",
};

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static int poll_cmd(void)
{
  if (SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk)
    return SIO->FIFO_RD;
  return 0;
}
static bool superBreak;
//-----------------------------------------------------------------------------
void capture_buffer()
{
  // pico-pio-usbの初期化
  pio_usb_configuration_t config = PIO_USB_DEFAULT_CONFIG;
  config.pin_dp = DP_INDEX;
  config.pio_tx_num = 0; // 必要に応じて設定
  config.pio_rx_num = 0; // 必要に応じて設定
  // 他に必要な設定があればここで追加

  pio_usb_host_init(&config);

  // メインループ: USBパケット受信はコールバックで処理される
  while (1) {
      // 必要に応じてコマンド受付やbreak条件をここで処理
      char command = poll_cmd();
      if (command == 'p') break;
      else if (command == 'z') { superBreak = true; break; }
      // pico-pio-usbのタスク処理（割り込み/ポーリング型の場合は不要なこともある）
  }
}

//-----------------------------------------------------------------------------
static void print_help(void)
{
  display_puts("\r\n-------------------------------------------------------------------\r\n");
  display_puts("USB Sniffer Lite. Built on "__DATE__" "__TIME__".\r\n");
  display_puts("\r\n");
  display_puts("Settings:\r\n");
  display_puts("  e - Capture speed       : "); display_puts(capture_speed_str[g_capture_speed]); display_puts("\r\n");
  display_puts("  g - Capture trigger     : "); display_puts(capture_trigger_str[g_capture_trigger]); display_puts("\r\n");
  display_puts("  l - Capture limit       : "); display_puts(capture_limit_str[g_capture_limit]); display_puts("\r\n");
  display_puts("  t - Time display format : "); display_puts(display_time_str[g_display_time]); display_puts("\r\n");
  display_puts("  a - Data display format : "); display_puts(display_data_str[g_display_data]); display_puts("\r\n");
  display_puts("  f - Fold empty frames   : "); display_puts(display_fold_str[g_display_fold]); display_puts("\r\n");
  display_puts("\r\n");
  display_puts("Commands:\r\n");
  display_puts("  h - Print this help message\r\n");
  display_puts("  b - Display buffer\r\n");
  display_puts("  s - Start capture\r\n");
  display_puts("  p - Stop capture\r\n");
  display_puts("  m - Toggle stream mode (逐次送信モード切替)\r\n");
  display_puts("\r\n");
}

//-----------------------------------------------------------------------------
static void change_setting(char *name, int *value, int count, const char *str[])
{
  (*value)++;

  if (*value == count)
    *value = 0;

  display_puts(name);
  display_puts(" changed to ");
  display_puts(str[*value]);
  display_puts("\r\n");
}

//-----------------------------------------------------------------------------
static void core1_main(void)
{
  HAL_GPIO_TRIGGER_in();
  HAL_GPIO_TRIGGER_pullup();

  while (1)
  {
    int cmd = poll_cmd();

    if (cmd == 's')
    {
      while(!superBreak)
        capture_buffer();
      superBreak = false;
    }
    else if (cmd == 'p')
      {} // Do nothing here, stop only works if the capture is running
    else if (cmd == 'b')
      display_buffer();
    else if (cmd == 'h' || cmd == '?')
      print_help();
    else if (cmd == 'e')
      change_setting("Capture speed", &g_capture_speed, CaptureSpeedCount, capture_speed_str);
    else if (cmd == 'g')
      change_setting("Capture trigger", &g_capture_trigger, CaptureTriggerCount, capture_trigger_str);
    else if (cmd == 'l')
      change_setting("Capture limit", &g_capture_limit, CaptureLimitCount, capture_limit_str);
    else if (cmd == 't')
      change_setting("Time display format", &g_display_time, DisplayTimeCount, display_time_str);
    else if (cmd == 'a')
      change_setting("Data display format", &g_display_data, DisplayDataCount, display_data_str);
    else if (cmd == 'f')
      change_setting("Fold empty frames", &g_display_fold, DisplayFoldCount, display_fold_str);
    else if (cmd == 'x')
      g_capture_speed = CaptureSpeed_Full;
    else if (cmd == 'y')
      g_capture_speed = CaptureSpeed_Low;
    else if (cmd == 'z')
      {} // Do nothing here, stop only works if the capture is running
  }
}

//-----------------------------------------------------------------------------
static void core1_start(void)
{
  static uint32_t core1_stack[CORE1_STACK_SIZE];
  uint32_t *stack_ptr = core1_stack + CORE1_STACK_SIZE;
  const uint32_t cmd[] = { 0, 1, (uint32_t)SCB->VTOR, (uint32_t)stack_ptr, (uint32_t)core1_main };

  while (SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk)
    (void)SIO->FIFO_RD;

  __SEV();

  while (SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk)
    (void)SIO->FIFO_RD;

  for (int i = 0; i < (int)(sizeof(cmd) / sizeof(uint32_t)); i++)
  {
    SIO->FIFO_WR = cmd[i];
    __SEV();

    while (0 == (SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk));
    (void)SIO->FIFO_RD;
  }
}

//-----------------------------------------------------------------------------
void capture_init(void)
{
  core1_start();
}

//-----------------------------------------------------------------------------
void capture_command(int cmd)
{
  if (SIO->FIFO_ST & SIO_FIFO_ST_RDY_Msk)
    SIO->FIFO_WR = cmd;
}

//----------------------------------------------------------------------------- 
void print_data(const char *pid, uint8_t *data, int size)
{
  // データ表示の実装例
  (void)pid;
  (void)data;
  (void)size;
  // 必要に応じて内容を記述
}
