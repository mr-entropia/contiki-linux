/*
 * Copyright (c) 2002, Adam Dunkels.
 * Copyright (c) 2026, Contiki Linux port.
 * All rights reserved.
 *
 * This file is part of the Contiki desktop environment.
 */

#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

void webserver_init(char *arg);

void webserver_log_file(u16_t *requester, char *file);

#endif /* __WEBSERVER_H__ */