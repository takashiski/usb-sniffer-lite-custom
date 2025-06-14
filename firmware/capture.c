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
static const uint16_t crc16_usb_tab[256] =
{
  0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
  0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
  0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
  0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
  0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
  0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
  0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
  0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
  0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
  0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
  0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
  0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
  0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
  0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
  0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
  0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
  0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
  0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
  0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
  0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
  0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
  0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
  0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
  0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
  0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
  0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
  0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
  0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
  0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
  0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
  0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
  0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040,
};

static const uint8_t crc5_usb_tab[256] =
{
  0x00, 0x0e, 0x1c, 0x12, 0x11, 0x1f, 0x0d, 0x03, 0x0b, 0x05, 0x17, 0x19, 0x1a, 0x14, 0x06, 0x08,
  0x16, 0x18, 0x0a, 0x04, 0x07, 0x09, 0x1b, 0x15, 0x1d, 0x13, 0x01, 0x0f, 0x0c, 0x02, 0x10, 0x1e,
  0x05, 0x0b, 0x19, 0x17, 0x14, 0x1a, 0x08, 0x06, 0x0e, 0x00, 0x12, 0x1c, 0x1f, 0x11, 0x03, 0x0d,
  0x13, 0x1d, 0x0f, 0x01, 0x02, 0x0c, 0x1e, 0x10, 0x18, 0x16, 0x04, 0x0a, 0x09, 0x07, 0x15, 0x1b,
  0x0a, 0x04, 0x16, 0x18, 0x1b, 0x15, 0x07, 0x09, 0x01, 0x0f, 0x1d, 0x13, 0x10, 0x1e, 0x0c, 0x02,
  0x1c, 0x12, 0x00, 0x0e, 0x0d, 0x03, 0x11, 0x1f, 0x17, 0x19, 0x0b, 0x05, 0x06, 0x08, 0x1a, 0x14,
  0x0f, 0x01, 0x13, 0x1d, 0x1e, 0x10, 0x02, 0x0c, 0x04, 0x0a, 0x18, 0x16, 0x15, 0x1b, 0x09, 0x07,
  0x19, 0x17, 0x05, 0x0b, 0x08, 0x06, 0x14, 0x1a, 0x12, 0x1c, 0x0e, 0x00, 0x03, 0x0d, 0x1f, 0x11,
  0x14, 0x1a, 0x08, 0x06, 0x05, 0x0b, 0x19, 0x17, 0x1f, 0x11, 0x03, 0x0d, 0x0e, 0x00, 0x12, 0x1c,
  0x02, 0x0c, 0x1e, 0x10, 0x13, 0x1d, 0x0f, 0x01, 0x09, 0x07, 0x15, 0x1b, 0x18, 0x16, 0x04, 0x0a,
  0x11, 0x1f, 0x0d, 0x03, 0x00, 0x0e, 0x1c, 0x12, 0x1a, 0x14, 0x06, 0x08, 0x0b, 0x05, 0x17, 0x19,
  0x07, 0x09, 0x1b, 0x15, 0x16, 0x18, 0x0a, 0x04, 0x0c, 0x02, 0x10, 0x1e, 0x1d, 0x13, 0x01, 0x0f,
  0x1e, 0x10, 0x02, 0x0c, 0x0f, 0x01, 0x13, 0x1d, 0x15, 0x1b, 0x09, 0x07, 0x04, 0x0a, 0x18, 0x16,
  0x08, 0x06, 0x14, 0x1a, 0x19, 0x17, 0x05, 0x0b, 0x03, 0x0d, 0x1f, 0x11, 0x12, 0x1c, 0x0e, 0x00,
  0x1b, 0x15, 0x07, 0x09, 0x0a, 0x04, 0x16, 0x18, 0x10, 0x1e, 0x0c, 0x02, 0x01, 0x0f, 0x1d, 0x13,
  0x0d, 0x03, 0x11, 0x1f, 0x1c, 0x12, 0x00, 0x0e, 0x06, 0x08, 0x1a, 0x14, 0x17, 0x19, 0x0b, 0x05,
};

static const char *capture_speed_str[CaptureSpeedCount] =
{
  [CaptureSpeed_Low]  = "Low",
  [CaptureSpeed_Full] = "Full",
};

static const char *capture_trigger_str[CaptureTriggerCount] =
{
  [CaptureTrigger_Enabled]  = "Enabled",
  [CaptureTrigger_Disabled] = "Disabled",
};

static const char *capture_limit_str[CaptureLimitCount] =
{
  [CaptureLimit_100]       = "100 packets",
  [CaptureLimit_200]       = "200 packets",
  [CaptureLimit_500]       = "500 packets",
  [CaptureLimit_1000]      = "1000 packets",
  [CaptureLimit_2000]      = "2000 packets",
  [CaptureLimit_5000]      = "5000 packets",
  [CaptureLimit_10000]     = "10000 packets",
  [CaptureLimit_Unlimited] = "Unlimited",
};

static const char *display_time_str[DisplayTimeCount] =
{
  [DisplayTime_First]    = "Relative to the first packet",
  [DisplayTime_Previous] = "Relative to the previous packet",
  [DisplayTime_SOF]      = "Relative to the SOF",
  [DisplayTime_Reset]    = "Relative to the bus reset",
};

static const char *display_data_str[DisplayDataCount] =
{
  [DisplayData_Full]    = "Full",
  [DisplayData_Limit16] = "Limit to 16 bytes",
  [DisplayData_Limit64] = "Limit to 64 bytes",
  [DisplayData_None]    = "Do not display data",
};

static const char *display_fold_str[DisplayFoldCount] =
{
  [DisplayFold_Enabled]  = "Enabled",
  [DisplayFold_Disabled] = "Disabled",
};

/*- Variables ---------------------------------------------------------------*/
uint32_t g_buffer[BUFFER_SIZE];
buffer_info_t g_buffer_info;

int g_capture_speed   = CaptureSpeed_Full;
int g_capture_trigger = CaptureTrigger_Disabled;
int g_capture_limit   = CaptureLimit_Unlimited;
int g_display_time    = DisplayTime_SOF;
int g_display_data    = DisplayData_Full;
int g_display_fold    = DisplayFold_Enabled;

static int g_rd_ptr    = 0;
static int g_wr_ptr    = 0;
static int g_sof_index = 0;
static bool g_may_fold = false;
int g_capture_stream_mode = 0; // 0:バッファ一括, 1:逐次送信

void capture_set_stream_mode(int mode) { g_capture_stream_mode = mode; }

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static uint16_t crc16_usb(uint8_t *data, int size)
{
  uint16_t crc = 0xffff;

  for (int i = 0; i < size; i++)
    crc = crc16_usb_tab[(crc ^ data[i]) & 0xff] ^ (crc >> 8);

  return crc;
}

//-----------------------------------------------------------------------------
static uint8_t crc5_usb(uint8_t *data, int size)
{
  uint8_t crc = 0xff;

  for (int i = 0; i < size; i++)
    crc = crc5_usb_tab[(crc ^ data[i]) & 0xff] ^ (crc >> 8);

  return crc;
}

//-----------------------------------------------------------------------------
static void handle_folding(int pid, uint32_t error)
{
  if (error)
  {
    g_buffer_info.errors++;
    set_error(true);
  }

  if (pid == Pid_Sof)
  {
    g_buffer_info.frames++;

    if (g_may_fold)
    {
      g_buffer[g_sof_index] |= CAPTURE_MAY_FOLD;
      g_buffer_info.folded++;
    }

    g_sof_index = g_wr_ptr-2;
    g_may_fold = true;
  }
  else if (pid != Pid_In && pid != Pid_Nak)
  {
    g_may_fold = false;
  }

  if (error)
    g_may_fold = false;
}

//-----------------------------------------------------------------------------
static void process_packet(int size)
{
  uint8_t *out_data = (uint8_t *)&g_buffer[g_wr_ptr];
  uint32_t v = 0x80000000;
  uint32_t error = 0;
  int out_size = 0;
  int out_bit = 0;
  int out_byte = 0;
  int stuff_count = 0;
  int pid, npid;

  while (size)
  {
    uint32_t w = g_buffer[g_rd_ptr++];
    int bit_count;

    if (size < 31)
    {
      w <<= (30-size);
      bit_count = size;
    }
    else
    {
      bit_count = 31;
    }

    v ^= (w ^ (w << 1));

    for (int i = 0; i < bit_count; i++)
    {
      int bit = (v & 0x80000000) ? 0 : 1;

      v <<= 1;

      if (stuff_count == 6)
      {
        if (bit)
          error |= CAPTURE_ERROR_STUFF;

        stuff_count = 0;
        continue;
      }
      else if (bit)
        stuff_count++;
      else
        stuff_count = 0;

      out_byte |= (bit << out_bit);
      out_bit++;

      if (out_bit == 8)
      {
        out_data[out_size++] = out_byte;
        out_byte = 0;
        out_bit = 0;
      }
    }

    size -= bit_count;
  }

  if (out_bit)
    error |= CAPTURE_ERROR_NBIT;

  if (out_size < 1)
  {
    error |= CAPTURE_ERROR_SIZE;
    return;
  }

  if (out_data[0] != (g_buffer_info.fs ? 0x80 : 0x81))
    error |= CAPTURE_ERROR_SYNC;

  if (out_size < 2)
  {
    error |= CAPTURE_ERROR_SIZE;
    return;
  }

  pid = out_data[1] & 0x0f;
  npid = (~out_data[1] >> 4) & 0x0f;

  if ((pid != npid) || (pid == Pid_Reserved))
    error |= CAPTURE_ERROR_PID;

  if (pid == Pid_Sof || pid == Pid_In || pid == Pid_Out || pid == Pid_Setup || pid == Pid_Ping || pid == Pid_Split)
  {
    if (((pid == Pid_Split) && (out_size != 5)) || ((pid != Pid_Split) && (out_size != 4)))
      error |= CAPTURE_ERROR_SIZE;
    else if (crc5_usb(&out_data[2], out_size-2) != 0x09)
      error |= CAPTURE_ERROR_CRC;
  }
  else if (pid == Pid_Data0 || pid == Pid_Data1 || pid == Pid_Data2 || pid == Pid_MData)
  {
    if (out_size < 4)
      error |= CAPTURE_ERROR_SIZE;
    else if (crc16_usb(&out_data[2], out_size-2) != 0xb001)
      error |= CAPTURE_ERROR_CRC;
  }

  handle_folding(pid, error);

  g_buffer[g_wr_ptr-2] = error | out_size;
  g_wr_ptr += (out_size + 3) / 4;
}

//-----------------------------------------------------------------------------
static uint32_t start_time(uint32_t end_time, uint32_t size)
{
  if (g_buffer_info.fs)
    return end_time - ((size * 5461) >> 16); // Divide by 12
  else
    return end_time - ((size * 43691) >> 16); // Divide by 1.5
}

//-----------------------------------------------------------------------------
static void process_buffer(void)
{
  uint32_t time_offset = start_time(g_buffer[1], g_buffer[0]);
  int out_count = 0;

  g_rd_ptr = 0;
  g_wr_ptr = 0;
  g_sof_index = 0;
  g_may_fold = false;

  g_buffer_info.errors = 0;
  g_buffer_info.resets = 0;
  g_buffer_info.frames = 0;
  g_buffer_info.folded = 0;

  for (int i = 0; i < g_buffer_info.count; i++)
  {
    uint32_t size = g_buffer[g_rd_ptr];
    uint32_t time = start_time(g_buffer[g_rd_ptr+1], size);

    if (size > 0xffff)
    {
      //display_puts("Synchronization error. Check your speed setting.\r\n");
      out_count = 0;
      break;
    }

    g_buffer[g_wr_ptr+1] = time - time_offset;
    g_rd_ptr += 2;
    g_wr_ptr += 2;
    out_count++;

    if (size == 0)
    {
      g_buffer[g_wr_ptr-2] = CAPTURE_RESET;
      handle_folding(-1, 0); // Prevent folding of resets
      g_buffer_info.resets++;
    }
    else if (size == 1)
    {
      if (g_buffer_info.fs)
      {
        out_count--; // Discard the packet
        g_wr_ptr -= 2;
      }
      else
      {
        g_buffer[g_wr_ptr-2] = CAPTURE_LS_SOF;
        handle_folding(Pid_Sof, 0); // Fold on LS SOFs
      }

      g_rd_ptr++;
    }
    else
    {
      process_packet(size-1);
    }
  }

  g_buffer_info.count = out_count;
}

//-----------------------------------------------------------------------------
static int capture_limit_value(void)
{
  if (g_capture_limit == CaptureLimit_100)
    return 100;
  else if (g_capture_limit == CaptureLimit_200)
    return 200;
  else if (g_capture_limit == CaptureLimit_500)
    return 500;
  else if (g_capture_limit == CaptureLimit_1000)
    return 1000;
  else if (g_capture_limit == CaptureLimit_2000)
    return 2000;
  else if (g_capture_limit == CaptureLimit_5000)
    return 5000;
  else if (g_capture_limit == CaptureLimit_10000)
    return 10000;
  else
    return 100000;
}

//-----------------------------------------------------------------------------
static int poll_cmd(void)
{
  if (SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk)
    return SIO->FIFO_RD;
  return 0;
}

//-----------------------------------------------------------------------------
static bool wait_for_trigger(void)
{
  if (!g_buffer_info.trigger)
    return true;

  display_puts("Waiting for a trigger\r\n");

  while (1)
  {
    if (poll_cmd() == 'p')
      return false;

    if (HAL_GPIO_TRIGGER_read() == 0)
      return true;
  }
}

static bool superBreak;

//-----------------------------------------------------------------------------
static void display_packet_direct(int wr_ptr, int time_offset) {
    int flags = g_buffer[wr_ptr];
    int time  = g_buffer[wr_ptr+1];
    int ftime = time - time_offset;
    int size  = flags & CAPTURE_SIZE_MASK;
    uint8_t *payload = (uint8_t *)&g_buffer[wr_ptr+2];
    int pid = payload[1] & 0x0f;
    // 必要に応じてprint_packetのロジックを移植
    // ここでは簡易的にDATA/SETUP/IN/OUT/ACK/NAK/STALLのみ出力
    if (flags & CAPTURE_ERROR_MASK) return;
    if (pid == Pid_Sof) {
        int frame = ((payload[3] << 8) | payload[2]) & 0x7ff;
        display_puts("SOF #");
        display_putdec(frame, 0);
        display_puts("\r\n");
    } else if (pid == Pid_In) {
        print_in_out_setup("IN", payload);
    } else if (pid == Pid_Out) {
        print_in_out_setup("OUT", payload);
    } else if (pid == Pid_Setup) {
        print_in_out_setup("SETUP", payload);
    } else if (pid == Pid_Ack) {
        print_handshake("ACK");
    } else if (pid == Pid_Nak) {
        print_handshake("NAK");
    } else if (pid == Pid_Stall) {
        print_handshake("STALL");
    } else if (pid == Pid_Data0) {
        print_data("DATA0", payload, size);
    } else if (pid == Pid_Data1) {
        print_data("DATA1", payload, size);
    } else if (pid == Pid_Data2) {
        print_data("DATA2", payload, size);
    } else if (pid == Pid_MData) {
        print_data("MDATA", payload, size);
    }
}

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
