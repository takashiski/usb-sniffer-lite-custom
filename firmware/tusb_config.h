#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#define CFG_TUSB_MCU             OPT_MCU_RP2040
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS              OPT_OS_NONE
#endif
#define CFG_TUSB_RHPORT0_MODE    (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define BOARD_TUD_RHPORT         0
#define CFG_TUD_CDC              1
#define CFG_TUD_MSC              0
#define CFG_TUD_HID              0
#define CFG_TUD_MIDI             0
#define CFG_TUD_VENDOR           0
#define CFG_TUD_ECM_RNDIS        0

// CDCバッファサイズ（TinyUSBのデフォルト値に合わせる）
#define CFG_TUD_CDC_RX_BUFSIZE   64
#define CFG_TUD_CDC_TX_BUFSIZE   64

#include "tusb_option.h"

#endif // _TUSB_CONFIG_H_
