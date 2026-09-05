/*
 * Copyright (c) 2002, Adam Dunkels.
 * Copyright (c) 2026, Contiki Linux port.
 * All rights reserved.
 *
 * This file is part of the Contiki desktop environment.
 *
 * Main entry point for the Linux port of Contiki. It replaces the
 * AVR port's contiki-main.c: instead of initializing AVR registers,
 * the RTL8019AS Ethernet controller and the timer0 interrupt, it
 * initializes the host clock, sets up the fixed private-network IP
 * address and starts the TAP interface device driver that sends and
 * receives Ethernet frames through the host's TAP interface.
 *
 * Usage: contiki [tap-device]
 */

#include "ctk.h"
#include "ctk-draw.h"
#include "ctk-vncserver.h"
#include "ek.h"

#include "uiplib.h"
#include "uip.h"
#include "uip_arp.h"
#include "tapdev.h"
#include "resolv.h"

#include "clock.h"

#include "webserver.h"
#include "program-handler.h"
#include "about-dsc.h"
#include "processes-dsc.h"
#include "calc-dsc.h"
#include "www-dsc.h"
#include "webserver-dsc.h"
#include "weblinks-dsc.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tapdev-drv.h"

static const struct uip_eth_addr ethaddr =
  {{0x00, 0x06, 0x98, 0x01, 0x02, 0x29}};

static u16_t addr[2];

/*---------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
  clock_init();

  ek_init();

  uip_init();
  tcpip_init(NULL);

  resolv_init(NULL);

  uip_setethaddr(ethaddr);

  /* Fixed IP configuration on the private 172.16.0.0/24 network. */
  uip_ipaddr(addr, 172, 16, 0, 2);
  uip_sethostaddr(addr);

  uip_ipaddr(addr, 172, 16, 0, 1);
  uip_setdraddr(addr);

  uip_ipaddr(addr, 255, 255, 255, 0);
  uip_setnetmask(addr);

  /* The DNS server is our host, for which the TAP interface has a
     dnsmasq or similar running. */
  uip_ipaddr(addr, 172, 16, 0, 1);
  resolv_conf(addr);

  /* Initialize the TAP device driver. */
  tapdev_init(argc > 1? argv[1]: NULL);
  tapdev_drv_init(NULL);

  ctk_init();

  ctk_vncserver_init(NULL);

  program_handler_init();

  webserver_init(NULL);

  program_handler_add(&calc_dsc, "Calculator", 0);
  program_handler_add(&weblinks_dsc, "Web links", 1);

  program_handler_add(&www_dsc, "Web browser", 1);
  program_handler_add(&webserver_dsc, "Web server", 1);
  program_handler_add(&processes_dsc, "Processes", 1);
  program_handler_add(&about_dsc, "About", 1);

  fprintf(stderr,
	  "contiki: started, VNC server on 172.16.0.2:5900 "
	  "(tap %s)\n",
	  argc > 1? argv[1]: TAPDEV_CONF_DEVICE_NAME);

  /* Main event loop. The ek_run() function processes poll handlers
     and events and never returns. Because the TAP device is opened
     in non-blocking mode, this loop runs at full speed and the poll
     handlers drive the uIP periodic processing and the packet
     input/output. A short sleep keeps the loop from spinning at
     100% CPU. */
  while(1) {
    ek_run();
    usleep(1000);
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
char *shell_prompt_text = "contiki-linux> ";

unsigned char
uip_fw_forward(void)
{
  return 0;
}

void
uip_fw_periodic(void)
{

}
/*---------------------------------------------------------------------------*/