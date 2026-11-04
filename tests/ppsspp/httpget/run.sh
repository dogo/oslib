#!/bin/sh
# Builds httpget, starts server.py, runs it in the PPSSPP flatpak with WLAN enabled, prints
# result.txt and compares it with expected.txt. Needs PSP networking, so not on a real PSP
# without a Wi-Fi profile. Restores PPSSPP's ini (--appendconfig persists).
set -u
cd "$(dirname "$0")"
rm -f *.elf *.prx EBOOT.PBP PARAM.SFO # relink: the makefile doesn't track the installed libosl.a
make -s >/dev/null || exit 1
G=$HOME/.var/app/org.ppsspp.PPSSPP/config/ppsspp/PSP/GAME
INI=$G/../SYSTEM/ppsspp.ini
cp "$INI" ini.bak
rm -rf "$G/httpget" && mkdir "$G/httpget" && cp EBOOT.PBP "$G/httpget/"
printf '[Network]\nEnableWlan = True\n' > "$G/httpget/net.ini"
python3 -I server.py 8123 2>server.log & SERVER=$!
flatpak run org.ppsspp.PPSSPP --appendconfig="$G/httpget/net.ini" "$G/httpget/EBOOT.PBP" >/dev/null 2>&1 & EMU=$!
for i in $(seq 60); do sleep 1; grep -q DONE "$G/httpget/result.txt" 2>/dev/null && break; done
pkill -x PPSSPPSDL; kill $SERVER $EMU 2>/dev/null; wait 2>/dev/null
cp ini.bak "$INI"; rm -f ini.bak
cp "$G/httpget/result.txt" result.txt 2>/dev/null || echo "no result" > result.txt
rm -rf "$G/httpget"
cat result.txt
[ -f expected.txt ] || { echo "no expected.txt: copy result.txt once it is right"; exit 0; }
diff expected.txt result.txt >/dev/null && echo "matches expected.txt" || { echo "DIFFERS from expected.txt"; exit 1; }
