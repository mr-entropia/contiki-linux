#!/usr/bin/env python3
"""
inet-agent.py - host-side Internet relay for Contiki.

Runs in the INITIAL network namespace (where the machine has real
Internet). It receives DNS queries and HTTP requests from the
userns-side gateway (inet-gateway.py) over Unix sockets in /tmp and
satisfies them using the host's resolver and sockets.

DNS responses are rewritten so that every A record points to the
gateway address (172.16.0.1); Contiki therefore connects to the
gateway, which forwards the HTTP traffic here.
"""

import os
import re
import socket
import struct
import threading

DNS_GW    = "/tmp/contiki-dns-gw.sock"
DNS_AGENT = "/tmp/contiki-dns-agent.sock"
HTTP_SOCK = "/tmp/contiki-http.sock"
GATEWAY   = "172.16.0.1"

def read_resolvers():
    servers = []
    try:
        for line in open("/etc/resolv.conf"):
            if line.strip().startswith("nameserver"):
                servers.append(line.split()[1])
    except OSError:
        pass
    if not servers:
        servers = ["1.1.1.1", "8.8.8.8"]
    return servers

def patch_a_records(msg, ipbytes):
    """Replace the IP address of every type-A resource record with
    ipbytes. Name compression is handled by processing sequentially."""
    out = bytearray(msg)
    if len(out) < 12:
        return bytes(out)
    qd, an, ns, ar = struct.unpack("!HHHH", out[4:12])

    pos = 12
    # questions
    for _ in range(qd):
        while pos < len(out) and out[pos] != 0:
            lb = out[pos]
            pos += 1 + lb
        pos += 1  # terminating zero
        pos += 4  # qtype + qclass

    # answers / authority / additional
    for sect in (an, ns, ar):
        for _ in range(sect):
            if pos >= len(out):
                return bytes(out)
            nb = out[pos]
            if nb & 0xC0 == 0xC0:
                pos += 2    # compressed name
            else:
                while pos < len(out) and out[pos] != 0:
                    lb = out[pos]
                    pos += 1 + lb
                pos += 1    # terminating zero
            if pos + 10 > len(out):
                return bytes(out)
            rtype = struct.unpack("!H", out[pos:pos+2])[0]
            rdlength = struct.unpack("!H", out[pos+8:pos+10])[0]
            rdata_off = pos + 10
            if rdata_off + rdlength > len(out):
                return bytes(out)
            if rtype == 1 and rdlength == 4:
                out[rdata_off:rdata_off+4] = ipbytes
            pos = rdata_off + rdlength

    return bytes(out)

def dns_agent():
    agent = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    try:
        os.unlink(DNS_AGENT)
    except OSError:
        pass
    agent.bind(DNS_AGENT)
    agent.settimeout(5.0)
    print("inet-agent: DNS relay ready (%s)" % DNS_AGENT, flush=True)

    resolvers = read_resolvers()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)
    target = (resolvers[0], 53)

    while True:
        try:
            query, gwaddr = agent.recvfrom(2048)
        except socket.timeout:
            continue
        except OSError:
            continue
        try:
            sock.sendto(query, target)
            response, _ = sock.recvfrom(4096)
            response = patch_a_records(response, socket.inet_aton(GATEWAY))
            agent.sendto(response, gwaddr)
        except socket.timeout:
            pass
        except Exception as e:
            print("inet-agent: dns err %r" % e, flush=True)

def http_agent():
    try:
        os.unlink(HTTP_SOCK)
    except OSError:
        pass
    srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    srv.bind(HTTP_SOCK)
    srv.listen(16)
    srv.settimeout(0.5)
    print("inet-agent: HTTP relay ready (%s)" % HTTP_SOCK, flush=True)

    while True:
        try:
            conn, _ = srv.accept()
        except socket.timeout:
            continue
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

        host = None
        for line in request.split(b"\r\n"):
            low = line.strip().lower()
            if low.startswith(b"host:"):
                host = line.split(b":", 1)[1].strip()
                break
        if not host:
            conn.close()
            return
        if b":" in host:
            host = host.split(b":")[0]
        host = host.decode("utf-8", "replace").strip()

        try:
            out = socket.create_connection((host, 80), timeout=20)
        except OSError:
            print("inet-agent: cannot connect to %r" % host, flush=True)
            conn.sendall(b"HTTP/1.0 502 Bad Gateway\r\ncontent-type: text/plain\r\n\r\n502 bad gateway\r\n")
            conn.close()
            return
        out.settimeout(20)
        out.sendall(request)
        while True:
            chunk = out.recv(8192)
            if not chunk:
                break
            conn.sendall(chunk)
        out.close()
    except OSError as e:
        print("inet-agent: http err %r" % e, flush=True)
    finally:
        try:
            conn.close()
        except OSError:
            pass

def main():
    threading.Thread(target=dns_agent, daemon=True).start()
    threading.Thread(target=http_agent, daemon=True).start()
    ev = threading.Event()
    try:
        while True:
            ev.wait(3600)
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    main()