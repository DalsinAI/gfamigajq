#!/bin/sh
# PC test for scripts/write-part.sh: the Part file GFOpenUpTool reads.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
mkdir -p "$T/C" "$T/Libs" "$T/Docs/jq"
: > "$T/C/jq"; : > "$T/Libs/regex.library"; : > "$T/Docs/jq/ReadMe"; : > "$T/Docs/jq/jq.1"
sh "$HERE/../scripts/write-part.sh" "$T" 1.8.1
fail=0
expect() { grep -qxF "$1" "$T/Part" || { echo "FAIL: no line: $1"; fail=1; }; }
expect 'part jq 1.8.1'
expect 'needs 68040'
expect 'copy "C/jq" "SYS:C/jq"'
expect 'copy "Docs/jq/ReadMe" "SYS:Docs/jq/ReadMe"'
expect 'copy "Docs/jq/jq.1" "SYS:Docs/jq/jq.1"'
expect 'add "Libs/regex.library" "LIBS:regex.library"'
[ "$(head -1 "$T/Part")" = 'part jq 1.8.1' ] || { echo "FAIL: first line"; fail=1; }
[ "$(grep -c '^copy\|^add' "$T/Part")" = 4 ] || { echo "FAIL: file count (the Part file itself must not be listed)"; fail=1; }
sh "$HERE/../scripts/write-part.sh" "$T" 1.8.1   # a second run: the same file
[ "$(grep -c '^copy\|^add' "$T/Part")" = 4 ] || { echo "FAIL: second run lists the Part file"; fail=1; }
[ $fail = 0 ] && echo "Part file: all checks passed"
exit $fail
