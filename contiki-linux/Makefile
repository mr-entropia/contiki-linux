# Copyright (c) 2002, Adam Dunkels.
# Copyright (c) 2026, Contiki Linux port.
# All rights reserved.
#
# This file is part of the Contiki desktop environment.
#
# Makefile for the Contiki Linux port.
#
# Build:   make
# Clean:   make clean

CONTIKI=../contiki
CONTIKILINUX=..

CC=gcc
CFLAGS=-O2 -g -fcommon -Wall -Wno-unused \
	-I. -Iconf -Ictk -Ilib -Iuip -Iapps \
	-I$(CONTIKI)/apps -I$(CONTIKI)/ctk -I$(CONTIKI)/ek \
	-I$(CONTIKI)/lib -I$(CONTIKI)/uip \
	-DWITH_UIP -DWITH_ASCII

# Source files local to the port.
LOCAL_FILES=contiki-main.o clock.o tapdev.o tapdev-drv.o

# Files from the portable part of Contiki.
EK=ek.o ek-service.o arg.o
UIP=uip.o uip_arch.o uiplib.o resolv.o uip_arp.o tcpip.o uip-split.o \
    uipbuf.o
LIB=timer.o strncasecmp.o libconio.o
CTK=ctk.o ctk-vncserver.o vnc-server.o vnc-out.o ctk-vncfont.o

# Applications.
APPS=httpd.o httpd-cgi.o httpd-fs.o webserver.o \
      webclient.o http-strings.o http-user-agent-string.o htmlparser.o \
      html-strings.o www.o www-dsc.o \
      weblinks.o weblinks-dsc.o \
      about.o about-dsc.o \
      processes.o processes-dsc.o calc.o calc-dsc.o \
      netconf.o netconf-dsc.o \
      webserver-dsc.o program-handler.o

OBJS=$(LOCAL_FILES) $(EK) $(UIP) $(LIB) $(CTK) $(APPS)

contiki: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: ctk/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: uip/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: apps/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: $(CONTIKI)/ek/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: $(CONTIKI)/apps/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: $(CONTIKI)/ctk/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: $(CONTIKI)/lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: $(CONTIKI)/uip/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o contiki