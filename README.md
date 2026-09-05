# contiki-linux
Contiki operating system ported from AVR to Linux.

Ported from `contiki-1.2-devel0` from 2004, intended for AVR 8-bit microcontrollers on Ethernet devboard.

# How to use
1. Install TigerVNC
2. In `/contiki-linux` run `make`
3. Run `./run-contiki.sh --client vncviewer 172.16.0.2:5900`

# Under the hood
This Linux port will create an unprivileged user namespace TAP by the name of `contiki0` and Contiki will run there.

A userspace Internet relay has been tested but probably does not work yet.
