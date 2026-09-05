/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * Copyright (c) 2026, Contiki Linux port.
 * All rights reserved.
 *
 * This file is part of the Contiki operating system.
 *
 * Clock implementation for the Linux port. The clock is based on
 * the host system clock (CLOCK_MONOTONIC), which is queried with
 * gettimeofday()/clock_gettime(). CLOCK_CONF_SECOND is 1000, i.e.,
 * clock_time() returns values in units of milliseconds.
 */

#include "clock.h"

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static struct timeval base;

void
clock_init(void)
{
  gettimeofday(&base, NULL);
}

clock_time_t
clock_time(void)
{
  struct timeval now;

  gettimeofday(&now, NULL);
  return (now.tv_sec - base.tv_sec) * 1000 +
    (now.tv_usec - base.tv_usec) / 1000;
}

/* Legacy interfaces from the AVR clock-conf.h that are not used on
   Linux but referenced by some modules. */

void
clock_delay(unsigned int us2)
{
  usleep(us2);
}

void
clock_wait(int ms10)
{
  usleep(ms10 * 10000);
}

void
clock_set_seconds(unsigned long s)
{
  base.tv_sec = (time_t)s;
}

unsigned long
clock_seconds(void)
{
  return (unsigned long)time(NULL);
}