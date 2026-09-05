/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * Copyright (c) 2026, Contiki Linux port.
 * All rights reserved.
 *
 * This file is part of the Contiki operating system.
 *
 * TAP device implementation for the Linux port.
 *
 * The device is created/attached via the TUN/TAP ioctl on
 * /dev/net/tun, which works both with root privileges and inside an
 * unprivileged user network namespace. The interface name is given at
 * run time (default: TAPDEV_CONF_DEVICE_NAME, "contiki0") and is
 * expected to already exist, or to be creatable by the running user.
 *
 * Reading and writing is done through a single non-blocking file
 * descriptor, polled from the Contiki poll handler in tapdev-drv.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include "tapdev.h"

static int tapfd = -1;

/*-----------------------------------------------------------------------------------*/
static int
tap_open(const char *ifname)
{
  int fd;
  struct ifreq ifr;

  fd = open("/dev/net/tun", O_RDWR | O_NONBLOCK);
  if(fd < 0) {
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);

  if(ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
    int err = errno;
    close(fd);
    errno = err;
    return -1;
  }

  return fd;
}
/*-----------------------------------------------------------------------------------*/
void
tapdev_init(const char *ifname)
{
  if(ifname == NULL) {
    ifname = TAPDEV_CONF_DEVICE_NAME;
  }

  tapfd = tap_open(ifname);
  if(tapfd < 0) {
    perror("contiki: could not open TAP device");
    fprintf(stderr,
	    "contiki: make sure that /dev/net/tun exists and that\n"
	    "contiki: interface %s can be created (see run-contiki.sh)\n",
	    ifname);
    exit(1);
  }
}
/*-----------------------------------------------------------------------------------*/
int
tapdev_read(unsigned char *buf, int len)
{
  int r;

  r = read(tapfd, buf, len);
  if(r < 0) {
    if(errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
      return 0;
    }
    perror("contiki: TAP read");
    return 0;
  }
  return r;
}
/*-----------------------------------------------------------------------------------*/
int
tapdev_write(const void *buf, int len)
{
  int w;

  w = write(tapfd, buf, len);
  if(w < 0) {
    if(errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
      return 0;
    }
    perror("contiki: TAP write");
    return 0;
  }
  return w;
}
/*-----------------------------------------------------------------------------------*/
void
tapdev_poll(unsigned char *buf, int maxlen,
	    int (*handler)(unsigned char *buf, int len))
{
  int r;

  r = tapdev_read(buf, maxlen);
  if(r > 0 && handler != NULL) {
    handler(buf, r);
  }
}
/*-----------------------------------------------------------------------------------*/