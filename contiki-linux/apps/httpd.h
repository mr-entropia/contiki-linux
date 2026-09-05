/*
 * Copyright (c) 2001, Adam Dunkels.
 * Copyright (c) 2026, Contiki Linux port.
 * All rights reserved.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * The HTTP server header for the Linux port. The AVR version declared
 * prog_char pointers (flash memory); on Linux all data is in ordinary
 * memory and prog_char is an alias for const unsigned char.
 */

#ifndef __HTTPD_H__
#define __HTTPD_H__

#include "contiki.h"
#include "avr/pgmspace.h"

void httpd_init(void);
void httpd_appcall(void *state);

struct httpd_state {
  u8_t state;
  u16_t count;
  u8_t poll;
  prog_char *dataptr;
  prog_char *script;
};

extern struct httpd_state *hs;
#endif /* __HTTPD_H__ */