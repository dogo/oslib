#!/usr/bin/env bash
#
# Format the project's C sources with Uncrustify.
#
# Usage:
#   ./run-uncrustify.sh              format every tracked source in src/ and samples/
#   ./run-uncrustify.sh --check      report files that need formatting, change nothing
#   ./run-uncrustify.sh FILE...      format only the given files
#
# Only files tracked by git are considered, so build artifacts and submodules
# are excluded automatically. Paths listed in .uncrustifyignore are skipped too.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UNCRUSTIFY_CFG="${SCRIPT_DIR}/uncrustify.cfg"
IGNORE_FILE="${SCRIPT_DIR}/.uncrustifyignore"
ROOTS=(src samples)

command -v uncrustify >/dev/null 2>&1 || {
	echo "error: uncrustify not found in PATH" >&2
	exit 127
}
[ -f "$UNCRUSTIFY_CFG" ] || {
	echo "error: missing $UNCRUSTIFY_CFG" >&2
	exit 1
}

check_only=false
explicit_files=()
for arg in "$@"; do
	case "$arg" in
		--check) check_only=true ;;
		-h|--help) sed -n '3,12p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
		-*) echo "error: unknown option '$arg'" >&2; exit 2 ;;
		*) explicit_files+=("$arg") ;;
	esac
done

cd "$SCRIPT_DIR"

# Collect candidate files, repo-relative and NUL-separated.
collect() {
	if [ ${#explicit_files[@]} -gt 0 ]; then
		printf '%s\0' "${explicit_files[@]}"
	elif git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
		# Tracked files only: skips build output and submodule contents.
		local roots=() r
		for r in "${ROOTS[@]}"; do roots+=("$r/*.c" "$r/*.h"); done
		git ls-files -z -- "${roots[@]}"
	else
		find "${ROOTS[@]}" -type f \( -name '*.c' -o -name '*.h' \) -print0
	fi
}

# Read ignore patterns (blank lines and # comments skipped).
ignored=()
if [ -f "$IGNORE_FILE" ]; then
	while IFS= read -r line || [ -n "$line" ]; do
		[[ -z "${line// }" || "$line" =~ ^[[:space:]]*# ]] && continue
		ignored+=("${line%/}")
	done < "$IGNORE_FILE"
fi

is_ignored() {
	local path="$1" pat
	for pat in ${ignored[@]+"${ignored[@]}"}; do
		# Prefix match on the repo-relative path, or an explicit glob.
		[[ "$path" == "$pat" || "$path" == "$pat"/* || "$path" == $pat ]] && return 0
	done
	return 1
}

files=()
while IFS= read -r -d '' f; do
	is_ignored "$f" || files+=("$f")
done < <(collect)

if [ ${#files[@]} -eq 0 ]; then
	echo "No files to process."
	exit 0
fi

if [ "$check_only" = true ]; then
	unformatted=()
	tmp="$(mktemp)"
	trap 'rm -f "$tmp"' EXIT
	for f in "${files[@]}"; do
		uncrustify -c "$UNCRUSTIFY_CFG" -f "$f" -q -o "$tmp" 2>/dev/null
		cmp -s "$tmp" "$f" || unformatted+=("$f")
	done
	if [ ${#unformatted[@]} -gt 0 ]; then
		echo "The following ${#unformatted[@]} of ${#files[@]} file(s) are not formatted:" >&2
		printf '  %s\n' "${unformatted[@]}" >&2
		echo >&2
		echo "Run ./run-uncrustify.sh to fix them." >&2
		exit 1
	fi
	echo "All ${#files[@]} file(s) are correctly formatted."
	exit 0
fi

echo "Running uncrustify on ${#files[@]} files..."
uncrustify -c "$UNCRUSTIFY_CFG" --no-backup --replace -q "${files[@]}"
echo "Done!"
