#!/usr/bin/env python3
"""Exercise run.sh with fake tools and private paths; never launch PPSSPP or change HOME."""
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


def executable(path, body):
    path.write_text(body)
    path.chmod(0o755)


MOCK = """import os, pathlib, signal, sys, time
root = pathlib.Path(os.environ['RUNNER_TEST_ROOT'])
mode = os.environ['RUNNER_TEST_MODE']
if pathlib.Path(sys.argv[0]).name == 'python3':
    (root / 'server.pid').write_text(str(os.getpid()))
else:
    if sys.argv[1] == 'kill':
        (root / 'killed').write_text(sys.argv[2])
        os.kill(int((root / 'emu.pid').read_text()), signal.SIGTERM)
        sys.exit()
    if '--instance-id-fd=3' in sys.argv:
        os.write(3, b'12345\\n')
    ini = root / 'SYSTEM/ppsspp.ini'
    ini.write_text('changed')
    (root / 'emu.pid').write_text(str(os.getpid()))
    if mode not in ('INT', 'TERM'):
        result = 'DONE\\n' if mode != 'error' else 'FAIL\\nDONE\\n'
        (root / 'GAME/httpget/result.txt').write_text(result)
    (root / 'started').touch()
while True:
    time.sleep(60)
"""


def check(mode):
    with tempfile.TemporaryDirectory(prefix="oslib-runner-") as temp:
        root = Path(temp)
        tools = root / "bin"
        tools.mkdir()
        (root / "GAME").mkdir()
        (root / "SYSTEM").mkdir()
        ini = root / "SYSTEM/ppsspp.ini"
        if mode != "backup-error":
            ini.write_text("original")
        testdir = root / "GAME/httpget"
        if mode == "existing":
            testdir.mkdir()
            (testdir / "keep").write_text("user file")
        # Relocate only the installed path; HOME and all real tools remain untouched.
        script = Path(__file__).with_name("run.sh").read_text()
        script = script.replace(
            "G=$HOME/.var/app/org.ppsspp.PPSSPP/config/ppsspp/PSP/GAME",
            'G="' + str(root / "GAME") + '"',
        )
        (root / "run.sh").write_text(script)
        (root / "expected.txt").write_text("DONE\n")
        executable(tools / "make", "#!/bin/sh\n: > EBOOT.PBP\n")
        executable(tools / "sleep", "#!/bin/sh\n/bin/sleep 0.02\n")
        executable(tools / "pkill", '#!/bin/sh\ntouch "$RUNNER_TEST_ROOT/global-kill"\n')
        executable(tools / "cp", '''#!/bin/sh
if [ "$RUNNER_TEST_MODE" = restore-error ]; then
    case $1 in */ini.bak) exit 1 ;; esac
fi
exec /bin/cp "$@"
''')
        for name in ("python3", "flatpak"):
            executable(tools / name, "#!" + sys.executable + "\n" + MOCK)
        env = dict(os.environ, PATH=str(tools) + os.pathsep + os.environ["PATH"],
                   RUNNER_TEST_ROOT=str(root), RUNNER_TEST_MODE=mode)
        process = subprocess.Popen(["sh", str(root / "run.sh")], env=env,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   start_new_session=True)
        try:
            if mode in ("INT", "TERM"):
                deadline = time.monotonic() + 5
                while not (root / "started").exists():
                    assert process.poll() is None, "runner exited before launch"
                    assert time.monotonic() < deadline, "runner never launched"
                    time.sleep(0.01)
                process.send_signal(getattr(signal, "SIG" + mode))
            out, err = process.communicate(timeout=10)
            expected = {"normal": 0, "INT": 130, "TERM": 143}.get(mode, 1)
            assert process.returncode == expected, (mode, process.returncode, out, err)
            assert not (root / "global-kill").exists(), "killed unrelated emulators"
            if mode == "existing":
                assert (testdir / "keep").read_text() == "user file"
            elif mode == "restore-error":
                assert (testdir / "ini.bak").read_text() == "original"
                assert b"backup kept" in err
            else:
                assert not testdir.exists(), (mode, "test directory left behind")
            if mode == "restore-error":
                assert ini.read_text() == "changed"
            elif mode != "backup-error":
                assert ini.read_text() == "original", (mode, "INI not restored")
            else:
                assert not ini.exists(), "restored a nonexistent backup"
            if mode not in ("existing", "backup-error"):
                assert (root / "killed").read_text() == "12345", "wrong Flatpak instance"
            for name in ("server", "emu"):
                path = root / (name + ".pid")
                if path.exists():
                    try:
                        os.kill(int(path.read_text()), 0)
                    except ProcessLookupError:
                        continue
                    raise AssertionError(mode + ": child left running: " + name)
        finally:
            # Also contain an older/broken runner while demonstrating regression failures.
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            for name in ("server", "emu"):
                path = root / (name + ".pid")
                if path.exists():
                    try:
                        os.kill(int(path.read_text()), signal.SIGKILL)
                    except ProcessLookupError:
                        pass


for case in ("normal", "error", "INT", "TERM", "existing", "backup-error", "restore-error"):
    check(case)
print("runner cleanup: PASS")
