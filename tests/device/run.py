#!/usr/bin/env python3
"""Build the device tests and run them on a PSP over USB with PSPLink.

The PSP must be running PSPLink with the cable plugged in (not in USB mode).
PSPLink runs each test from its own directory on host0:, so its result.txt
(and screen.png, if it saves one) lands in tests/device/<test>/ on the PC.
Both are compared with the expected.txt/expected.png recorded on a real PSP;
--update records them instead, after an intended change. Exits non-zero if any
test fails, differs from its expected output, writes no result or hangs.

    tests/device/run.py [--update] [--port 10000] [test ...]
"""
import argparse
import difflib
import os
import shutil
import signal
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
TIMEOUT = 60  # seconds for one test to finish and PSPLink to reconnect


def compare(t, update):
    """Problems with this test's outputs against its expected ones (none if --update)."""
    problems = []
    for out, exp in (("result.txt", "expected.txt"), ("screen.png", "expected.png")):
        out, exp = os.path.join(HERE, t, out), os.path.join(HERE, t, exp)
        if not os.path.exists(out):
            continue
        if update:
            shutil.copyfile(out, exp)
        elif not os.path.exists(exp):
            problems.append(f"no {os.path.basename(exp)} (record one with --update)")
        elif open(out, "rb").read() != open(exp, "rb").read():
            problems.append(f"differs from {os.path.basename(exp)}")
            if out.endswith(".txt"):
                problems += [l.rstrip("\n") for l in difflib.unified_diff(
                    open(exp).readlines(), open(out).readlines(), "expected", "result")]
    return problems


def shell_ready(port):
    """Wait until PSPLink's shell answers: right after a USB (re)connect it can drop commands."""
    end = time.time() + TIMEOUT
    while time.time() < end:
        r = subprocess.run(["pspsh", "-p", str(port), "-e", "ls"], cwd=HERE,
                           stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, timeout=10)
        if r.returncode == 0 and b"Listing" in r.stdout:
            return True
        time.sleep(0.5)
    return False


def main():
    # Run the finally below on SIGTERM too (e.g. from timeout), so usbhostfs_pc isn't left holding the port
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(1))
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=10000, help="usbhostfs_pc/pspsh base port")
    parser.add_argument("--update", action="store_true", help="record the outputs as the expected ones")
    parser.add_argument("tests", nargs="*", help="test directories (default: all)")
    args = parser.parse_args()
    tests = args.tests or sorted(d for d in os.listdir(HERE) if os.path.exists(os.path.join(HERE, d, "makefile")))

    for t in tests:
        # -B: the makefiles don't depend on libosl.a, so force a relink against the installed one
        if subprocess.run(["make", "-B", "-C", os.path.join(HERE, t)], stdout=subprocess.DEVNULL).returncode:
            sys.exit(f"{t}: build failed")

    hostfs = subprocess.Popen(["stdbuf", "-oL", "usbhostfs_pc", "-b", str(args.port), "."], cwd=HERE,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    connected = threading.Semaphore(0)
    bind_failed = threading.Event()

    def watch():
        for line in hostfs.stdout:
            if "Connected to device" in line:
                connected.release()
            elif line.startswith("bind:"):  # it keeps going without the shell port
                bind_failed.set()
                connected.release()
    threading.Thread(target=watch, daemon=True).start()

    failed = []
    try:
        if not connected.acquire(timeout=TIMEOUT):
            sys.exit("PSP not found: start PSPLink on it with the USB cable plugged in")
        if bind_failed.is_set():
            sys.exit(f"port {args.port} is in use (another usbhostfs_pc?): pass --port")
        for t in tests:
            if not shell_ready(args.port):
                sys.exit("PSPLink's shell does not answer")
            for f in ("result.txt", "screen.png"):
                if os.path.exists(os.path.join(HERE, t, f)):
                    os.remove(os.path.join(HERE, t, f))
            subprocess.run(["pspsh", "-p", str(args.port), "-e", f"./{t}/{t}.prx"], cwd=HERE,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=TIMEOUT)
            # sceKernelExitGame resets PSPLink, which reconnects: that's the end of the test.
            finished = connected.acquire(timeout=TIMEOUT)
            result = os.path.join(HERE, t, "result.txt")
            text = open(result).read() if os.path.exists(result) else ""
            ok = finished and text and "FAIL" not in text and " NO" not in text
            problems = compare(t, args.update) if ok else []
            ok = ok and not problems
            print(f"{'PASS' if ok else 'FAIL'} {t}" + ("" if finished else " (timed out)"))
            print("".join("    " + line + "\n" for line in text.splitlines() + problems), end="")
            if not ok:
                failed.append(t)
            if not finished:
                break  # PSPLink is gone; the rest would only time out too
    finally:
        hostfs.terminate()
        hostfs.wait()  # free the ports before a next run starts
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
