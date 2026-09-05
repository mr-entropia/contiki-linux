#!/usr/bin/env python3
"""
Minimal RFB 3.3 (VNC) test client used to verify the Contiki VNC
server. It performs the full handshake, requests a framebuffer update
and parses the BGR233 pixel data, printing a textual rendering of the
screen. Then it sends a couple of key events and a pointer event to
exercise the input path.

Usage: rfb-test.py <host> <port>
"""

import socket
import struct
import sys
import time

def recvn(sock, n, timeout=10):
    data = b""
    sock.settimeout(timeout)
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            raise EOFError("connection closed, got %d of %d bytes" % (len(data), n))
        data += chunk
    return data

class PixelFormat:
    def __init__(self, fmt):
        self.bpp = fmt[0]
        self.depth = fmt[1]
        self.endian = fmt[2]   # 1 big, 0 little
        self.truecolor = fmt[3]
        rmax, gmax, bmax = struct.unpack("!HHH", fmt[4:10])
        self.red_max, self.green_max, self.blue_max = rmax, gmax, bmax
        self.red_shift, self.green_shift, self.blue_shift, _ = struct.unpack("!BBBB", fmt[10:14])
        self.size = max(1, (self.bpp + 7) // 8)

    def raw_bytes(self, n):
        size = self.size
        if self.endian == 1:  # big endian, pixel in high-order bytes
            return bytes(n)
        if self.endian == 0 and size == 4:
            return bytes(n)
        return bytes(n)

    def lum(self, data, off):
        v = 0
        if self.endian == 1:
            for i in range(self.size):
                v = (v << 8) | data[off + i]
        else:
            for i in range(self.size - 1, -1, -1):
                v = (v << 8) | data[off + i]
        thr = self.red_max or 1
        tgg = self.green_max or 1
        tbb = self.blue_max or 1
        r = ((v >> self.red_shift) & self.red_max) / thr
        g = ((v >> self.green_shift) & self.green_max) / tgg
        b = ((v >> self.blue_shift) & self.blue_max) / tbb
        return 0.30 * r + 0.59 * g + 0.11 * b

def make_pf(fmt):
    return PixelFormat(fmt)

def handle_fb_update(sock, pf):
    mtype, pad, nrects = struct.unpack("!BBH", recvn(sock, 4))
    assert mtype == 0, "expected framebuffer update, got type %d" % mtype
    rects = []
    pixels = []
    for _ in range(nrects):
        recthdr = recvn(sock, 12)
        x, y, w, h, enc = struct.unpack("!HHHHI", recthdr)
        if enc == 0:  # RAW
            data = recvn(sock, w * h * pf.size)
            pixels.append((x, y, w, h, data))
        elif enc == 2:  # RRE
            subrects, = struct.unpack("!I", recvn(sock, 4))
            bg = recvn(sock, pf.size)
            pixels.append((x, y, w, h, bg * (w * h)))
            for _ in range(subrects):
                colour = recvn(sock, pf.size)
                r = struct.unpack("!HHHH", recvn(sock, 8))
                px = colour * (r[2] * r[3])
                pixels.append((r[0], r[1], r[2], r[3], px))
        else:
            raise RuntimeError("unexpected encoding %d at rect x=%d y=%d w=%d h=%d" %
                               (enc, x, y, w, h))
        rects.append((x, y, w, h, enc))
    return rects, pixels

def dump_raw(sock, host, port, width, height, nbytes=160):
    """Reconnect and dump raw bytes of the first update."""
    s = socket.create_connection((host, port), timeout=10)
    s.settimeout(10)
    s.recv(12)
    s.sendall(b"RFB 003.003\n")
    s.recv(4)
    s.sendall(b"\x01")
    s.recv(4 + 16 + 4 + 4)  # serverinit
    s.sendall(struct.pack("!BBHHHH", 3, 0, 0, 0, width, height))
    data = b""
    while len(data) < nbytes:
        chunk = s.recv(nbytes - len(data))
        if not chunk:
            break
        data += chunk
    return data

def render(pixels, width, height, pf):
    screen = bytearray(b" " * (width * height))
    for (x, y, w, h, data) in pixels:
        for row in range(h):
            for col in range(w):
                idx = (y + row) * width + (x + col)
                if idx < len(screen):
                    lum = pf.lum(data, (row * w + col) * pf.size)
                    screen[idx] = int(255 * lum)
    return screen

RAMP = " .:-=+*#%@"

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else "172.16.0.2"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 5900

    sock = socket.create_connection((host, port), timeout=10)
    sock.settimeout(10)

    version = recvn(sock, 12)
    print("server version:", version)
    sock.sendall(b"RFB 003.003\n")

    auth = recvn(sock, 4)
    print("security types:", auth)
    sock.sendall(b"\x01")

    init = recvn(sock, 4 + 16 + 4)
    width, height = struct.unpack("!HH", init[:4])
    fmt = init[4:20]
    namelen = struct.unpack("!I", init[20:24])[0]
    name = recvn(sock, namelen)
    bps, depth, endian, truecolor = fmt[0], fmt[1], fmt[2], fmt[3]
    redmax, greenmax, bluemax = struct.unpack("!HHH", fmt[4:10])
    rshift, gshift, bshift, pad1 = struct.unpack("!BBBB", fmt[10:14])
    print("framebuffer %dx%d name=%r" % (width, height, name))
    print("pixelformat bpp=%d depth=%d endian=%d truecolor=%d "
          "r=%d/%d g=%d/%d b=%d/%d" %
          (bps, depth, endian, truecolor,
           redmax, rshift, greenmax, gshift, bluemax, bshift))
    pf = make_pf(fmt)

    req = struct.pack("!BBHHHH", 3, 0, 0, 0, width, height)
    sock.sendall(req)

    # Read the initial update. The server chunks it across multiple
    # update messages, so keep reading until we have covered a large
    # fraction of the screen (or clearly enough data).
    all_pixels = []
    all_rects = 0
    covered_area = 0
    first_msg = None
    msgs = 0
    try:
        for _ in range(200):
            rects, pixels = handle_fb_update(sock, pf)
            all_pixels += pixels
            all_rects += len(rects)
            covered_area += sum(w * h for (x, y, w, h, enc) in rects)
            if covered_area >= 0.4 * width * height:
                break
    except Exception as e:
        print("handshake/update error:", e)
        try:
            raise
        finally:
            raw = dump_raw(sock, host, port, width, height)
            print("RAW FIRST %d BYTES:" % len(raw))
            for i in range(0, len(raw), 16):
                chunk = raw[i:i+16]
                hexs = " ".join("%02x" % b for b in chunk)
                asc = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
                print("%04x  %-47s  %s" % (i, hexs, asc))
    print("got %d rectangles total" % all_rects)
    screen = render(all_pixels, width, height, pf)

    cellw, cellh = 6, 8
    ascii_rows = []
    for cy in range(0, height, cellh):
        row = ""
        for cx in range(0, width, cellw):
            total = 0.0
            samples = 0
            for yy in range(cy, min(cy + cellh, height)):
                for xx in range(cx, min(cx + cellw, width)):
                    total += screen[yy * width + xx]
                    samples += 1
            avg = total / samples if samples else 0
            row += RAMP[min(int(avg / 255.0 * len(RAMP)), len(RAMP) - 1)]
        ascii_rows.append(row.rstrip())
    for row in ascii_rows:
        print("|" + row + "|")

    for down in (1, 0):
        ev = struct.pack("!BBHI", 4, down, 0, 0x61)
        sock.sendall(ev)
    time.sleep(0.2)
    ev = struct.pack("!BBHH", 5, 1, 150, 120)
    sock.sendall(ev)
    time.sleep(0.2)
    ev = struct.pack("!BBHH", 5, 0, 150, 120)
    sock.sendall(ev)

    req = struct.pack("!BBHHHH", 3, 1, 0, 0, width, height)
    sock.sendall(req)
    rects2, pixels2 = handle_fb_update(sock, pf)
    print("got %d rectangles after input" % len(rects2))

    sock.close()
    print("RFB test PASSED")

if __name__ == "__main__":
    main()