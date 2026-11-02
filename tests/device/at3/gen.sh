#!/bin/sh
# Regenerates this test's AT3 files: a 2 s 440 Hz sine encoded with atracdenc
# (https://github.com/dcherednik/atracdenc, LGPL). Needs ffmpeg and python3.
#   tests/device/at3/gen.sh /path/to/atracdenc
set -e
E=${1:?usage: gen.sh /path/to/atracdenc}
cd "$(dirname "$0")"
ffmpeg -v error -y -f lavfi -i "sine=frequency=440:duration=2:sample_rate=44100" -ac 2 -c:a pcm_s16le tone.wav
"$E" -e atrac3 --bitrate 128 -i tone.wav -o lp2.at3 >/dev/null   # 132 kbps, align 384
"$E" -e atrac3 --bitrate 100 -i tone.wav -o lp3.at3 >/dev/null   # 105 kbps, align 304
"$E" -e atrac3_lp4 -i tone.wav -o lp4.at3 >/dev/null             # 66 kbps, align 192, joint stereo
"$E" -e atrac3plus -i tone.wav -o plus.at3 >/dev/null            # 352 kbps, align 2048
rm tone.wav
# atracdenc writes a 40-byte WAVE_FORMAT_EXTENSIBLE fmt chunk; Sony's tools (and
# OSLib, which reads the codec config from bytes 42-43) use 52 bytes: 01 00, then
# 0x2800 | (block align / 8 - 1) big-endian (0x28 = stereo), then 8 zero bytes.
python3 -I - plus.at3 <<'PY'
import struct, sys
d = bytearray(open(sys.argv[1], 'rb').read())
i = d.index(b'fmt ')
assert struct.unpack('<I', d[i+4:i+8])[0] == 40
align = struct.unpack('<H', d[i+8+12:i+8+14])[0]
ext = bytes([1, 0]) + struct.pack('>H', 0x2800 | (align // 8 - 1)) + bytes(8)
d[i+4:i+8] = struct.pack('<I', 52)
d[i+8+40:i+8+40] = ext
d[4:8] = struct.pack('<I', len(d) - 8)
open(sys.argv[1], 'wb').write(d)
PY
