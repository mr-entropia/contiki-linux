#!/usr/bin/env python3
"""End-to-end check of the Contiki Internet gateway: DNS via the
gateway must answer with 172.16.0.1, and HTTP must return a real
response relayed through the host agent."""
import socket
import struct
import sys
import time

GATEWAY = "172.16.0.1"
HOST = "example.com"

def build_query(name):
    qid = 0x1234
    header = struct.pack("!HHHHHH", qid, 0x0100, 1, 0, 0, 0)
    qname = b"" + b"".join(bytes([len(p)]) + p.encode() for p in name.split(".")) + b"\x00"
    return header + qname + struct.pack("!HH", 1, 1)

def parse_a(msg):
    qid, flags, qd, an, ns, ar = struct.unpack("!HHHHHH", msg[:12])
    pos = 12
    # skip question (name + qtype + qclass)
    while msg[pos] != 0:
        pos += 1 + msg[pos]
    pos += 1 + 4
    answers = []
    for _ in range(an):
        if msg[pos] & 0xC0 == 0xC0:
            pos += 2
        else:
            while msg[pos] != 0:
                pos += 1 + msg[pos]
            pos += 1
        rtype, rclass = struct.unpack("!HH", msg[pos:pos+4])
        pos += 8                      # rtype + rclass + ttl
        rdlen = struct.unpack("!H", msg[pos:pos+2])[0]
        pos += 2
        if rtype == 1 and rdlen == 4:
            answers.append(socket.inet_ntoa(msg[pos:pos+4]))
        pos += rdlen
    return qid, flags, answers

def dns_test():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(8)
    q = build_query(HOST)
    s.sendto(q, (GATEWAY, 53))
    data, _ = s.recvfrom(2048)
    print("DNS raw:", data[:120].hex())
    qid, flags, answers = parse_a(data)
    print("DNS: %s -> %s (rcode=%d)" % (HOST, answers, flags & 0xF))
    if answers and answers[0] == GATEWAY:
        print("DNS OK")
        return True
    print("DNS FAIL", answers)
    return False

def http_test():
    try:
        s = socket.create_connection((GATEWAY, 80), timeout=8)
    except OSError as e:
        print("HTTP connect fail", e)
        return False
    s.settimeout(8)
    req = ("GET / HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n" % HOST).encode()
    s.sendall(req)
    data = b""
    while True:
        chunk = s.recv(8192)
        if not chunk:
            break
        data += chunk
    s.close()
    print("HTTP: got %d bytes, head: %r" % (len(data), data[:40]))
    if data.startswith(b"HTTP/"):
        print("HTTP OK")
        return True
    print("HTTP FAIL")
    return False

ok = True
try:
    ok = dns_test() and ok
except Exception as e:
    print("DNS error:", e)
    ok = False
try:
    ok = http_test() and ok
except Exception as e:
    print("HTTP error:", e)
    ok = False
print("INET RESULT:", "PASS" if ok else "FAIL")
sys.exit(0 if ok else 1)