/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * Copyright (c) 2026, Contiki Linux port.
 * All rights reserved.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * The HTTP filesystem interface for the Linux port. On AVR, the file
 * data lives in program memory; on Linux it is plain memory.
 */

#ifndef __HTTPD_FS_H__
#define __HTTPD_FS_H__

#include "uip.h"

#define HTTPD_FS_STATISTICS 1

struct httpd_fs_file {
  prog_char *data;
  int len;
};

/* file must be allocated by caller and will be filled in
   by the function. */
int httpd_fs_open(const char *name, struct httpd_fs_file *file);

#if HTTPD_FS_STATISTICS
unsigned long httpd_fs_count(char *name);
unsigned long httpd_fs_total(void);
void httpd_fs_inc(void);
#endif /* HTTPD_FS_STATISTICS */

void httpd_fs_init(void);

#endif /* __HTTPD_FS_H__ */