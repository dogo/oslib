#!/bin/sh
# Regenerates this test's images. Needs python3 with PIL, and ImageMagick 7.
set -e
cd "$(dirname "$0")"
python3 -I - <<'PY'
from PIL import Image
# A 256-entry colour table using 16 colours, half of them past index 15 (248-255)
im = Image.new("P", (32, 24))
im.putpalette([c for i in range(256) for c in (i, 255 - i, (i * 7) & 255)])
im.putdata([(x % 8) + (248 if (y // 3) % 2 else 0) for y in range(24) for x in range(32)])
im.save("pal256.gif", optimize=False)
PY
magick pal256.gif -depth 8 rgba:pal256.rgba
# Two frames: the loader returns the last one
magick -size 32x24 xc:red xc:blue -loop 0 anim.gif
magick -size 32x24 xc:green PNG24:rgb.png
