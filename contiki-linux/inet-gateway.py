#!/usr/bin/env python3
"""
inet-gateway.py - userns-side Internet gateway for Contiki.

Runs INSIDE the Contiki user network namespace. It binds the DNS and
HTTP endpoints on 172.16.0.1 (the host side of the TAP) and relays to
the host-side agent (inet-agent.py) over Unix sockets, which have full
Internet access from the host network namespace.

Contiki cannot reach the Internet directly (the unprivileged netns has
no route out), so:
  - UDP 172.16.0.1:53   DNS queries are forwarded to the host agent,
                        which rewrites A records to 172.16.0.1 so that
                        Contiki connects to this gateway.
  - TCP 172.16.0.1:80   HTTP requests are forwarded to the host agent,
                        which fetches them from the real web server.
"""

import os
import re
import socket
import struct
import sys
import threading

DNS_AGENT         = "/tmp/contiki-dns-agent.sock"
DNS_GW            = "/tmp/contiki-dns-gw.sock"
HTTP_SOCK         = "/tmp/contiki-http.sock"
GATEWAY_IP        = "172.16.0.1"
HTTP_PORT         = 80
DNS_PORT          = 53

def xlate_ip4(ip):
    parts = [int(x) for x in ip.split(".")]
    return struct.pack("!BBBB", *parts)

def setup_udp():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((GATEWAY_IP, DNS_PORT))
    return s

def setup_tcp():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((GATEWAY_IP, HTTP_PORT))
    s.listen(8)
    return s

def dns_loop(udp):
    gw = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    try:
        os.unlink(DNS_GW)
    except OSError:
        pass
    gw.bind(DNS_GW)
    gw.settimeout(0.2)
    print("inet-gateway: DNS forwarder on %s:%d" % (GATEWAY_IP, DNS_PORT),
          flush=True)
    while True:
        try:
            query, addr = udp.recvfrom(2048)
        except OSError:
            continue
        try:
            gw.sendto(query, DNS_AGENT)
            gw.settimeout(3.0)
            response, _ = gw.recvfrom(2048)
            udp.sendto(response, addr)
        except socket.timeout:
            pass
        except OSError as e:
            if e.errno != 11:
                print("inet-gateway: dns fwd error: %r" % e, flush=True)
        finally:
            gw.settimeout(0.2)

def http_loop(tcp):
    print("inet-gateway: HTTP forwarder on %s:%d" % (GATEWAY_IP, HTTP_PORT),
          flush=True)
    while True:
        try:
            conn, addr = tcp.accept()
        except OSError:
            continue
        threading.Thread(target=handle_http, args=(conn,), daemon=True).start()

def handle_http(conn):
    try:
        conn.settimeout(15.0)
        request = b""
        while b"\r\n\r\n" not in request and len(request) < 8192:
            chunk = conn.recv(4096)
            if not chunk:
                break
            request += chunk
        if not request:
            conn.close()
            return

        relay = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        relay.settimeout(15.0)
        relay.connect(HTTP_SOCK)
        relay.sendall(request)
        relay.shutdown(socket.SHUT_WR)

        while True:
            chunk = relay.recv(8192)
            if not chunk:
                break
            conn.sendall(chunk)
    except OSError as e:
        print("inet-gateway: http error: %r" % e, flush=True)
    finally:
        try:
            conn.close()
        except OSError:
            pass
        try:
            relay.close()
        except OSError:
            pass

def main():
    udp = setup_udp()
    tcp = setup_tcp()
    d = threading.Thread(target=dns_loop, args=(udp,), daemon=True)
    h = threading.Thread(target=http_loop, args=(tcp,), daemon=True)
    d.start()
    h.start()
    while True:
        sleep_hack = threading.Event()
        sleep_hack.wait(3600)

if __name__ == "__main__":
    main()