#!/usr/bin/env python3
"""TigerVNC-style test: handshake, 32bpp, incremental=1 first request.
Decode the framebuffer and report luminance stats. Expect non-zero
content after the fix."""
import socket, struct, sys

HOST = sys.argv[1] if len(sys.argv) > 1 else "172.16.0.2"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 5900

def recvn(s, n, t=15):
    d = b""
    s.settimeout(t)
    while len(d) < n:
        c = s.recv(n - len(d))
        if not c: raise EOFError("closed %d/%d" % (len(d), n))
        d += c
    return d

s = socket.create_connection((HOST, PORT), timeout=6)
recvn(s, 12); s.sendall(b"RFB 003.003\n")
recvn(s, 4); s.sendall(b"\x00")          # security reply like TigerVNC
init = recvn(s, 24)
w, h = struct.unpack("!HH", init[:4])
namelen = struct.unpack("!I", init[20:24])[0]
recvn(s, namelen)
print("server %dx%d" % (w, h))

# Already sent security reply above. Now SetPixelFormat(32bpp) +
# SetEncodings + INCREMENTAL request. NO extra shared flag byte.
spf = struct.pack("!Bxxx", 0) + struct.pack("!BBBBHHHBBBxH",32,24,0,1,65535,65535,65535,16,8,0,0)
encs = struct.pack("!BBH", 2,0,4) + struct.pack("!IIII",0,1,2,5)
req = struct.pack("!BBHHHH",3,1,0,0,w,h)   # incremental=1
s.sendall(spf + encs + req)

fw = bytearray(b"\x20" * (w * h))
msgs = 0
covered = 0
while msgs < 200 and covered < 0.5 * w * h:
    hdr = recvn(s, 4)
    mtype, pad, nrects = struct.unpack("!BBH", hdr)
    if mtype != 0:
        print("nonupdate type", mtype)
        break
    msgs += 1
    for i in range(nrects):
        x, yy, ww, hh, enc = struct.unpack("!HHHHI", recvn(s, 12))
        if enc == 0:
            data = recvn(s, ww * hh * 4)
            for r in range(hh):
                for c in range(ww):
                    off = (r + yy) * w + (c + x)
                    pix = struct.unpack("<I", data[(r*ww+c)*4:][:4])[0]
                    fw[off] = int(0.3*((pix>>16)&255) + 0.59*((pix>>8)&255) + 0.11*(pix&255))
            covered += ww * hh
        elif enc == 2:
            nsub = struct.unpack("!I", recvn(s, 4))[0]  # 4-byte count (TigerVNC)
            bg = struct.unpack("<I", recvn(s, 4))[0]
            bgv = int(0.3*((bg>>16)&255) + 0.59*((bg>>8)&255) + 0.11*(bg&255))
            for r in range(hh):
                for c in range(ww):
                    fw[(r+yy)*w + (c+x)] = bgv
            for _ in range(nsub):
                recvn(s, 4 + 8)
            covered += ww * hh
        else:
            print("unexpected enc", enc, "at", x, yy, ww, hh)
            sys.exit(2)
print("msgs=%d covered=%d/%d" % (msgs, covered, w*h))
vals = [px for px in fw if px != 0x20]
print("nonblank pixels=%d distinct=%d maxlum=%d" % (len(vals), len(set(fw)), max(fw)))
if len(vals) > 5000:
    print("RESULT: SCREEN CONTENT RECEIVED")
else:
    print("RESULT: STILL EMPTY")