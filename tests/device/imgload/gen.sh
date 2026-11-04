#!/bin/sh
# Regenerates this test's images and, for each, the RGBA pixels ImageMagick
# decodes from it (<name>.rgba, 32x24x4 bytes), which the test compares with
# what OSLib's loaders produce. Needs python3 and ImageMagick 7.
set -e
cd "$(dirname "$0")"
# 32x24: red/green gradients, blue checker, alpha ramp, a pure black and a pure white column
python3 -I -c '
import sys
out = bytearray()
for y in range(24):
    for x in range(32):
        if x == 0: out += bytes([0, 0, 0, 255])
        elif x == 31: out += bytes([255, 255, 255, 255])
        else: out += bytes([x * 8, y * 10, 255 if (x // 4 + y // 4) % 2 else 40, 255 - y * 10])
sys.stdout.buffer.write(out)' > src.rgba
M="magick -size 32x24 -depth 8 rgba:src.rgba"
$M PNG32:rgba8.png
$M -background white -alpha remove -alpha off PNG24:rgb8.png
$M -depth 16 PNG64:rgba16.png
$M -background white -alpha remove -alpha off -colors 200 PNG8:pal8.png
$M -colors 120 PNG8:pal8t.png                                   # palette + tRNS
$M -background white -alpha remove -alpha off -colors 16 -define png:bit-depth=4 -define png:color-type=3 pal4.png
$M -background white -alpha remove -alpha off -colorspace Gray -define png:color-type=0 gray8.png
$M -colorspace Gray -define png:color-type=4 graya8.png
$M -background white -alpha remove -alpha off -colorspace Gray -depth 4 -define png:color-type=0 -define png:bit-depth=4 gray4.png
$M -background white -alpha remove -alpha off -colorspace Gray -depth 16 -define png:color-type=0 gray16.png
$M -colorspace Gray -depth 16 -define png:color-type=4 graya16.png
$M -interlace PNG PNG32:interlaced.png
$M -background white -alpha remove -alpha off -quality 90 jpeg.jpg
$M -background white -alpha remove -alpha off -quality 90 -interlace JPEG progressive.jpg
$M -colors 64 gif.gif                                           # transparent index
# The docs say JPEGs above quality 90 may crash the loader
$M -background white -alpha remove -alpha off -quality 95 jpeg95.jpg
$M -background white -alpha remove -alpha off -quality 100 -sampling-factor 1x1 jpeg100.jpg   # 4:4:4
# The docs say images are limited to 512x512: a 600 px wide one (the pattern tiled)
magick -size 32x24 -depth 8 rgba:src.rgba -write mpr:t +delete -size 600x24 tile:mpr:t PNG32:wide.png
rm src.rgba
# Grey with a tRNS chunk on a mid grey (pixel 5,5 of gray8) rather than black
magick gray8.png -depth 8 gray:- | python3 -I -c '
import struct, sys, zlib
g = sys.stdin.buffer.read()
def chunk(t, b): return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b))
raw = b"".join(b"\0" + g[y * 32:(y + 1) * 32] for y in range(24))
sys.stdout.buffer.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 32, 24, 8, 0, 0, 0, 0))
    + chunk(b"tRNS", struct.pack(">H", g[5 * 32 + 5])) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))' > grayt.png
# A grey JPEG: one component, which the loader read as three
magick gray8.png -quality 90 -type Grayscale grayjpg.jpg
magick jpeg.jpg -colorspace CMYK cmyk.jpg   # libjpeg can't turn CMYK into RGB: rejected
# A 256-entry palette using 16 colours, half of them past index 15 (248-255), for OSL_PF_4BIT
python3 -I -c '
import struct, sys, zlib
def chunk(t, b): return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b))
plte = b"".join(bytes([i, 255 - i, (i * 7) & 255]) for i in range(256))
raw = b"".join(b"\0" + bytes((x % 8) + (248 if (y // 3) % 2 else 0) for x in range(32)) for y in range(24))
sys.stdout.buffer.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 32, 24, 8, 3, 0, 0, 0))
    + chunk(b"PLTE", plte) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))' > pal256.png
# The same with a tRNS chunk giving every entry its own alpha
python3 -I -c '
import struct, sys, zlib
def chunk(t, b): return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b))
plte = b"".join(bytes([i, 255 - i, (i * 7) & 255]) for i in range(256))
trns = bytes((i * 37) & 255 for i in range(256))
raw = b"".join(b"\0" + bytes((x % 8) + (248 if (y // 3) % 2 else 0) for x in range(32)) for y in range(24))
sys.stdout.buffer.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 32, 24, 8, 3, 0, 0, 0))
    + chunk(b"PLTE", plte) + chunk(b"tRNS", trns) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))' > pal256t.png
for f in *.png *.jpg *.gif; do [ "$f" = cmyk.jpg ] || magick "$f" -depth 8 "rgba:${f%.*}.rgba"; done
mv pal256.png pal256.rgba pal256t.png pal256t.rgba ../imgpal/   # only imgpal uses them
