#!/bin/sh
# Regenerates this test's SFont images: row 0 alternates the separator colour (pixel 0,0)
# and a glyph column, so every other column is one letter. Needs python3.
set -e
cd "$(dirname "$0")"
python3 -I - <<'PY'
import struct, zlib
def png(name, width):
    def chunk(t, b): return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b))
    sep, glyph = b"\xff\x00\xff\xff", b"\xff\xff\xff\xff"
    top = b"".join(glyph if x % 2 else sep for x in range(width))
    raw = b"\0" + top + b"\0" + glyph * width
    open(name, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, 2, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))
png("font10.png", 21)    # 10 letters
png("font300.png", 601)  # 300 letters: more than OSL_SFONT holds (256)
PY
