#!/bin/sh
# jq built for the PC the way build-amiga.sh builds it for GFOS (the same
# patches, the probe's jq-config.h, the one-task pthread.h, match() on
# GFOpenRegex's core), then upstream's own tests run on it:
#   tests/jq.test (the language), tests/onig.test (match, test, sub,
#   capture, scan, splits: through GFOpenRegex here), tests/man.test
#   (every example in the manual), tests/base64.test, tests/uri.test.
# Cases GFOpenRegex doesn't offer (look-behind, \p{..}) are listed in
# tests/onig-skips.txt and left out, each with its reason.
#   sh tests/host-jq.sh     (WORK=... keeps the build somewhere else)
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$HERE/jq.lock"
. "$HERE/scripts/jq-common.sh"
WORK=${WORK:-"$HERE/build-host"}; CC=${HOST_CC:-cc}
mkdir -p "$WORK"
SRC="$WORK/jq"
jq_source "$SRC"
[ -n "${RX_SRC:-}" ] || { fetch_at "$WORK/gfopenregex" "$RX_REPO" "$RX_COMMIT"; RX_SRC="$WORK/gfopenregex"; }
jq_generate "$SRC" "gfamigajq host test"
PROBE_CC="$CC" jq_probe "$SRC" "$WORK/jq-config.h"
INC="-include $WORK/jq-config.h -I$HERE/amiga/include -I$RX_SRC/include -I$RX_SRC/src -I$HERE/amiga -I$SRC -I$SRC/src -I$SRC/vendor"
O="$WORK/obj"; rm -rf "$O"; mkdir -p "$O"
objs=""
for f in $JQ_SRCS; do
    o="$O/$(echo "$f" | tr / _).o"
    # shellcheck disable=SC2086
    $CC -O1 -g -w -Werror=implicit-function-declaration -Werror=int-conversion $INC -c "$SRC/$f" -o "$o"
    objs="$objs $o"
done
for f in "$HERE/amiga/rx_names.c" "$HERE/amiga/pthread_one.c" "$HERE/amiga/amiga_paths.c" "$HERE/tests/host_regex.c" \
         "$RX_SRC/src/rx_api.c" "$RX_SRC/src/rx_compile.c" "$RX_SRC/src/rx_exec.c" "$RX_SRC/src/rx_unicode.c"; do
    o="$O/$(basename "$f").o"
    # shellcheck disable=SC2086
    $CC -O1 -g -w -I$RX_SRC/include -I$RX_SRC/src -I$HERE/amiga/include -I$HERE/amiga -c "$f" -o "$o"
    objs="$objs $o"
done
# shellcheck disable=SC2086
$CC -o "$WORK/jq-host" $objs -lm
echo "built: $("$WORK/jq-host" --version)"

fail=0
run() {
    if (cd "$SRC" && env PAGER=less "$WORK/jq-host" -L tests/modules --run-tests "$1" > "$WORK/$(basename "$1").log" 2>&1); then
        echo "$1: $(grep "tests passed" "$WORK/$(basename "$1").log")"
    else
        echo "FAIL $1:"; grep -B2 -A6 '^\*\*\*' "$WORK/$(basename "$1").log" | head -60; fail=1
    fi
}
run tests/jq.test
run tests/man.test
run tests/base64.test
run tests/uri.test
# onig.test less the cases regex.library can't run (onig-skips.txt)
python3 "$HERE/tests/skip_cases.py" "$SRC/tests/onig.test" "$HERE/tests/onig-skips.txt" > "$SRC/tests/onig-gf.test"
run tests/onig-gf.test
# amiga/amiga_env.c's list (environ on GFOS: env, $ENV)
$CC -std=gnu99 -Wall -I"$HERE/amiga" -o "$WORK/env_test" "$HERE/tests/env_test.c" "$HERE/amiga/amiga_env.c" && "$WORK/env_test" || fail=1
exit $fail
