/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * Copyright (c) 2026, Contiki Linux port.
 * All rights reserved.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * The HTTP filesystem data structs for the Linux port. prog_char is
 * an alias for const unsigned char (see avr/pgmspace.h).
 */

#ifndef __HTTPD_FSDATA_H__
#define __HTTPD_FSDATA_H__

#include "uipopt.h"

#include "avr/pgmspace.h"

struct httpd_fsdata_file {
  struct httpd_fsdata_file *next;
  prog_char *name;
  prog_char *data;
  int len;
};

struct httpd_fsdata_file_noconst {
  struct httpd_fsdata_file *next;
  prog_char *name;
  prog_char *data;
  int len;
};

#endif /* __HTTPD_FSDATA_H__ */