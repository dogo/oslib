#!/bin/sh
# Builds httpget, starts server.py, runs it in the PPSSPP flatpak with WLAN enabled, prints
# result.txt and compares it with expected.txt. Needs PSP networking, so not on a real PSP
# without a Wi-Fi profile. Restores PPSSPP's ini (--appendconfig persists).
set -eu
cd "$(dirname "$0")"
rm -f ./*.elf ./*.prx EBOOT.PBP PARAM.SFO # relink: the makefile doesn't track the installed libosl.a
make -s >/dev/null || exit 1
G=$HOME/.var/app/org.ppsspp.PPSSPP/config/ppsspp/PSP/GAME
INI=$G/../SYSTEM/ppsspp.ini
TESTDIR=$G/httpget
# Refuse an existing installation: only this run's directory may be removed.
mkdir "$TESTDIR"
if ! cp "$INI" "$TESTDIR/ini.bak"; then
	rm -f "$TESTDIR/ini.bak"
	rmdir "$TESTDIR"
	exit 1
fi
SERVER='' EMU=''
cleanup() {
	status=$?
	set +e
	trap - 0
	trap '' INT TERM
	# The launcher PID can differ from the sandbox's PID; stop only our instance.
	if [ -s "$TESTDIR/instance" ]; then
		instance=$(cat "$TESTDIR/instance")
		case $instance in ''|*[!0-9]*) ;; *) flatpak kill "$instance" 2>/dev/null || : ;; esac
	fi
	for pid in $SERVER $EMU; do kill "$pid" 2>/dev/null || :; done
	for _ in 1 2 3; do
		alive=0
		for pid in $SERVER $EMU; do kill -0 "$pid" 2>/dev/null && alive=1; done
		[ "$alive" -eq 0 ] && break
		sleep 1
	done
	for pid in $SERVER $EMU; do
		kill -KILL "$pid" 2>/dev/null || :
		wait "$pid" 2>/dev/null || :
	done
	if cp "$TESTDIR/ini.bak" "$INI"; then
		rm -rf "$TESTDIR"
	else
		echo "Could not restore $INI; backup kept at $TESTDIR/ini.bak" >&2
		status=1
	fi
	exit "$status"
}
trap cleanup 0
trap 'exit 130' INT
trap 'exit 143' TERM
cp EBOOT.PBP "$TESTDIR/"
printf '[Network]\nEnableWlan = True\n' > "$TESTDIR/net.ini"
python3 -I server.py 8123 2>server.log & SERVER=$!
flatpak run --instance-id-fd=3 org.ppsspp.PPSSPP --appendconfig="$TESTDIR/net.ini" "$TESTDIR/EBOOT.PBP" 3>"$TESTDIR/instance" >/dev/null 2>&1 & EMU=$!
for _ in $(seq 60); do sleep 1; grep -q DONE "$TESTDIR/result.txt" 2>/dev/null && break; done
cp "$TESTDIR/result.txt" result.txt 2>/dev/null || echo "no result" > result.txt
cat result.txt
[ -f expected.txt ] || { echo "no expected.txt: copy result.txt once it is right"; exit 0; }
if diff expected.txt result.txt >/dev/null; then
	echo "matches expected.txt"
else
	echo "DIFFERS from expected.txt"
	exit 1
fi
