/*
 * Copyright (c) 2001-2004, Adam Dunkels.
 * Copyright (c) 2026, Contiki Linux port.
 * All rights reserved.
 *
 * This file is part of the Contiki operating system.
 *
 * The packet-service Ethernet device driver for the Linux port. This
 * is a direct mirror of the RTL8019AS driver (rtl8019-drv.c) from the
 * AVR port: it registers the "Packet driver" service so that
 * tcpip_output() sends frames through the TAP interface, and it polls
 * the TAP interface from its poll handler, feeding received frames to
 * the uIP ARP/IP stack.
 *
 * The interface name can be overridden at run time:
 *
 *   contiki <tap-device-name>
 *
 * otherwise the default TAPDEV_CONF_DEVICE_NAME ("contiki0") is used.
 */

#include "packet-service.h"

#include <string.h>

#include "tapdev.h"

#include "uip.h"
#include "uip_arp.h"

static void output(u8_t *hdr, u16_t hdrlen, u8_t *data, u16_t datalen);
static int input(u8_t *buf, int len);

static const struct packet_service_state state =
  {
    PACKET_SERVICE_VERSION,
    output
  };

EK_EVENTHANDLER(eventhandler, ev, data);
EK_POLLHANDLER(pollhandler);
EK_PROCESS(proc, PACKET_SERVICE_NAME ": Linux TAP", EK_PRIO_HIGH,
	   eventhandler, pollhandler, (void *)&state);

/*---------------------------------------------------------------------------*/
LOADER_INIT_FUNC(tapdev_drv_init, arg)
{
  ek_service_start(PACKET_SERVICE_NAME, &proc);
}
/*---------------------------------------------------------------------------*/
static void
output(u8_t *hdr, u16_t hdrlen, u8_t *data, u16_t datalen)
{
  /* Build the Ethernet header in front of the IP packet, exactly like
     the RTL8019 driver does. */
  uip_arp_out();

  /* The payload is not required to reside in uip_buf - the packet
     service passes it as a separate data pointer (uip_appdata). The
     TAP device needs a contiguous frame, so copy the payload into
     place right after the Ethernet+IP+TCP headers, which end at
     uip_len - datalen. A memmove is safe even when the payload
     already is in the buffer. */
  if(datalen > 0 &&
     uip_len > (UIP_LLH_LEN + UIP_TCPIP_HLEN) &&
     ((struct uip_eth_hdr *)uip_buf)->type == HTONS(UIP_ETHTYPE_IP)) {
    memmove(&uip_buf[uip_len - datalen], data, datalen);
  }

  tapdev_write(uip_buf, uip_len);
}
/*---------------------------------------------------------------------------*/
static int
input(u8_t *buf, int len)
{
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

  uip_len = len;

  if(BUF->type == HTONS(UIP_ETHTYPE_IP)) {
    uip_arp_ipin();
    uip_len -= sizeof(struct uip_eth_hdr);
    tcpip_input();
  } else if(BUF->type == HTONS(UIP_ETHTYPE_ARP)) {
    uip_arp_arpin();
    if(uip_len > 0) {
      tapdev_write(uip_buf, uip_len);
    }
  }
  uip_len = 0;
  return 0;
}
/*---------------------------------------------------------------------------*/
EK_EVENTHANDLER(eventhandler, ev, data)
{
  switch(ev) {
  case EK_EVENT_INIT:
  case EK_EVENT_REPLACE:
    break;
  case EK_EVENT_REQUEST_REPLACE:
    ek_replace((struct ek_proc *)data, NULL);
    LOADER_UNLOAD();
    break;
  case EK_EVENT_REQUEST_EXIT:
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
EK_POLLHANDLER(pollhandler)
{
  tapdev_poll(uip_buf, UIP_BUFSIZE, input);
}
/*---------------------------------------------------------------------------*/