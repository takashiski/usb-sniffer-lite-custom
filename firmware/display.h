// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>

/*- Prototypes --------------------------------------------------------------*/
void display_putc(char c);
void display_puts(const char *s);
void display_puthex(uint32_t v, int size);
void display_putdec(uint32_t v, int size);

void display_buffer(void);
void print_in_out_setup(const char *type, uint8_t *payload);
void print_handshake(const char *type);
void print_data(const char *pid, uint8_t *data, int size);

#endif // _DISPLAY_H_
