#!/bin/sh
# Regenerates this test's WAV files: 0.25 s of a 440 Hz sine, 16-bit PCM.
set -e
cd "$(dirname "$0")"
w() { ffmpeg -v error -y -f lavfi -i "sine=frequency=440:duration=0.25:sample_rate=$1" -ac $2 -c:a pcm_s16le -map_metadata -1 -fflags +bitexact "$3"; }
w 44100 2 tone44s.wav
w 22050 1 tone22m.wav
w 48000 2 tone48s.wav
w 32000 2 tone32s.wav
