#!/usr/bin/env bash
#
# Run static analysis over the project with Cppcheck.
#
# Usage:
#   ./run-cppcheck.sh                 analyse the whole project
#   ./run-cppcheck.sh --report FILE   also write the findings to FILE
#   ./run-cppcheck.sh FILE...         limit the analysis to the given files
#
# Exits non-zero when anything is reported, so it works as a CI gate.
#
# The analysis is driven by compile_commands.json, which is what makes cppcheck
# see -DPSP and the include paths and therefore analyse the code that actually
# ships. Without it cppcheck guesses the preprocessor configuration and reasons
# about branches the PSP build never compiles.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SUPPRESSIONS="${SCRIPT_DIR}/.cppcheck-suppress"
COMPILE_DB="${SCRIPT_DIR}/build/compile_commands.json"

command -v cppcheck >/dev/null 2>&1 || {
	echo "error: cppcheck not found in PATH" >&2
	exit 127
}

report=""
filters=()
while [ $# -gt 0 ]; do
	case "$1" in
		--report)
			[ $# -ge 2 ] || { echo "error: --report needs a file name" >&2; exit 2; }
			report="$2"; shift 2 ;;
		-h|--help) sed -n '3,9p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
		-*) echo "error: unknown option '$1'" >&2; exit 2 ;;
		*) filters+=("--file-filter=*$1"); shift ;;
	esac
done

cd "$SCRIPT_DIR"

# compile_commands.json is generated, not tracked. Configure the project if it
# is not there yet; that needs the PSP toolchain the build already requires.
if [ ! -f "$COMPILE_DB" ]; then
	echo "compile_commands.json not found, configuring the project..."
	cmake --preset psp >/dev/null || {
		echo "error: could not configure the project. Is the PSP toolchain (PSPDEV) installed?" >&2
		exit 1
	}
fi

args=(
	--project="$COMPILE_DB"
	--platform=mips32            # the PSP is 32-bit; the default is the host
	--enable=warning,performance,portability
	--check-level=exhaustive     # do not give up on branch-heavy functions
	--inline-suppr               # honour // cppcheck-suppress comments
	--suppressions-list="$SUPPRESSIONS"
	--error-exitcode=1
	--quiet
	-j "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"
)

# --enable=all adds unusedFunction and style checks. For a library the former is
# meaningless (the public API is by definition unused internally) and together
# they drowned the real findings, so they stay off.

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

set +e
cppcheck "${args[@]}" ${filters[@]+"${filters[@]}"} 2> "$tmp"
status=$?
set -e

cat "$tmp" >&2
[ -n "$report" ] && cp "$tmp" "$report" && echo "Findings written to $report"

if [ $status -ne 0 ]; then
	echo >&2
	echo "Cppcheck reported problems (exit $status)." >&2
	exit $status
fi

echo "Cppcheck found no problems."
