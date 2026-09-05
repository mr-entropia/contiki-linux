/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * Copyright (c) 2026, Contiki Linux port.
 * All rights reserved.
 *
 * This file is part of the Contiki operating system.
 *
 * A minimal TAP device driver for the Linux port. The device is
 * opened by the user (or by the run script) and needs to have an IP
 * address assigned to be reachable. The driver reads and writes raw
 * Ethernet frames through the TAP interface.
 */

#ifndef TAPDEV_H_
#define TAPDEV_H_

#ifndef TAPDEV_CONF_DEVICE_NAME
#define TAPDEV_CONF_DEVICE_NAME "contiki0"
#endif /* TAPDEV_CONF_DEVICE_NAME */

void tapdev_init(const char *ifname);
int tapdev_read(unsigned char *buf, int len);
int tapdev_write(const void *buf, int len);
void tapdev_poll(unsigned char *buf, int maxlen,
		 int (*handler)(unsigned char *buf, int len));

#endif /* TAPDEV_H_ */