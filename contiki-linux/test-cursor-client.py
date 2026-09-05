#!/usr/bin/env python3
"""Verify the Contiki VNC server sends pointer cursor shapes: client
advertises the Cursor encoding (-239), moves the pointer, and checks
that a -239 pseudo-rectangle with an 8x8 cursor and mask arrives."""
import socket
import struct
import sys
import time

def dump_stream():
    import sys
    s2 = socket.create_connection((HOST, PORT), timeout=8)
    s2.settimeout(8)
    s2.recv(12); s2.sendall(b"RFB 003.003\n")
    s2.recv(4); s2.sendall(b"\x00")
    init = recvn(s2, 24)
    w, h = struct.unpack("!HH", init[:4])
    nl = struct.unpack("!I", init[20:24])[0]
    recvn(s2, nl)
    s2.sendall(spf); s2.sendall(encs)
    s2.sendall(struct.pack("!BBHHHH", 3, 0, 0, 0, w, h))
    d = b""
    while len(d) < 600:
        c = s2.recv(600 - len(d))
        if not c: break
        d += c
    for i in range(0, min(len(d), 600), 16):
        print("  %04x  %s" % (i, " ".join("%02x"%b for b in d[i:i+16])))
    s2.close()

HOST = sys.argv[1] if len(sys.argv) > 1 else "172.16.0.2"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 5900

def recvn(s, n, t=8):
    d = b""
    s.settimeout(t)
    while len(d) < n:
        c = s.recv(n - len(d))
        if not c:
            raise EOFError("closed %d/%d" % (len(d), n))
        d += c
    return d

s = socket.create_connection((HOST, PORT), timeout=8)
recvn(s, 12); s.sendall(b"RFB 003.003\n")
recvn(s, 4); s.sendall(b"\x00")
init = recvn(s, 24)
w, h = struct.unpack("!HH", init[:4])
namelen = struct.unpack("!I", init[20:24])[0]
recvn(s, namelen)
print("server %dx%d" % (w, h))

# 32bpp + encodings including the Cursor pseudo-encoding (-239)
spf = struct.pack("!Bxxx", 0) + struct.pack("!BBBBHHHBBBxH", 32, 24, 0, 1, 65535, 65535, 65535, 16, 8, 0, 0)
encs = struct.pack("!BBH", 2, 0, 5) + struct.pack("!IIIII", 0, 1, 2, 5, 0xFFFFFF11)
s.sendall(spf)
s.sendall(encs)
s.sendall(struct.pack("!BBHHHH", 3, 0, 0, 0, w, h))

# Move the pointer a few times to force cursor updates
for i in range(3):
    s.sendall(struct.pack("!BBHH", 5, 0, 100 + i * 10, 60))
    time.sleep(0.05)
s.sendall(struct.pack("!BBHHHH", 3, 1, 0, 0, w, h))

found_cursor = 0
cursor_sizes = []
s.settimeout(8)
end = time.time() + 8
while time.time() < end and found_cursor < 2:
    hdr = recvn(s, 4)
    mtype, pad, nrects = struct.unpack("!BBH", hdr)
    for _ in range(nrects):
        rh = recvn(s, 12)
        x, y, rw, rhh, enc = struct.unpack("!HHHHI", rh)
        if not (x == 0 and y == 0 and rw == 0 and rhh == 0) and enc & 0x80000000:
            pass
        if enc == 0xFFFFFF11:
            data = recvn(s, rw * rhh * 4)
            mask = recvn(s, ((rw + 7) // 8) * rhh)
            found_cursor += 1
            cursor_sizes.append((rw, rhh, mask.hex()))
        elif enc == 0:
            recvn(s, rw * rhh * 4)
        elif enc == 2:
            nsub, = struct.unpack("!I", recvn(s, 4))
            recvn(s, 4)
            for _ in range(nsub):
                recvn(s, 4 + 8)
        else:
            print("other enc", enc)
            dump_stream()
            break

print("cursor updates found:", found_cursor)
for sz in cursor_sizes:
    print("  cursor %dx%d mask=%s" % (sz[0], sz[1], sz[2]))
if found_cursor >= 1 and cursor_sizes[0][0:2] == (8, 8):
    print("CURSOR TEST PASSED")
else:
    print("CURSOR TEST FAILED")
s.close()