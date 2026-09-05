/*
 * Copyright (c) 2004, Adam Dunkels.
 * All rights reserved.
 *
 * This file is part of the Contiki Linux port.
 *
 * Configuration for the uIP TCP/IP stack on the Linux port.
 * The private subnet 172.16.0.0/24 is used on the virtual
 * TAP interface.
 */
#ifndef __UIP_CONF_H__
#define __UIP_CONF_H__

#define UIP_CONF_MAX_CONNECTIONS 40
#define UIP_CONF_MAX_LISTENPORTS 40
#define UIP_CONF_BUFFER_SIZE     1500

#define UIP_CONF_TCP_SPLIT       1

/* Ethernet frame length used by the TAP device. */
#define UIP_CONF_LLH_LEN         14

/* The native byte order of the host (little endian on x86). */
#define UIP_CONF_BYTE_ORDER      LITTLE_ENDIAN

#define UIP_CONF_UDP             1
#define UIP_CONF_UDP_CONNS       4

#define UIP_CONF_ARPTAB_SIZE     8

#define UIP_CONF_LOGGING         0
#define UIP_CONF_STATISTICS      0
#define UIP_CONF_RECEIVE_WINDOW  1500

#endif /* __UIP_CONF_H__ */