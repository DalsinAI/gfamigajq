#!/bin/sh
# Builds jq, the command-line JSON processor, for GFOS (AmigaOS 3.2.3 with
# GFOpenUp) with a stove: 68040, soft float with the FPU taken when there
# is one (fpuauto). Its regular expressions (match, test, sub, capture,
# scan, splits) go through GFOpenRegex's regex.library in Oniguruma's place.
#   STOVE=.../stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
#   (RX_SRC=... to use a GFOpenRegex tree already on disk)
# jq, GFOpenRegex and fpuauto are fetched at the commits jq.lock gives;
# nothing third-party is kept here. The PC tests (tests/host-jq.sh: jq's
# own test files on the same sources) run first when there is a PC
# compiler; HOST_TESTS=0 skips them.
# OUT gets Parts/jq: a GFOpenUp part (its Part file, C:jq, LIBS:regex.library,
# the manual and licences in SYS:Docs/jq), installed with
#   GFOpenUpTool INSTALL Parts/jq
# and tests/: jq's test files, for running jq --run-tests in a lab.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/jq.lock"
. "$HERE/scripts/jq-common.sh"
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
# outside the checkout: Cradle cleans it (git clean -ffdx) before every job
WORK="${WORK:-$HOME/.cache/gfamigajq/build}"; OUT="${OUT:-$WORK/out}"; JOBS=${JOBS:-8}
TC="$STOVE/prefix"; PATH="$TC/bin:$PATH"; export PATH
mkdir -p "$WORK/deps" "$OUT"; OUT=$(cd "$OUT" && pwd)
t0=$(date +%s)

# --- the PC tests first: the same sources, jq's own test files ----------------
if [ "${HOST_TESTS:-1}" != 0 ] && command -v "${HOST_CC:-cc}" > /dev/null 2>&1; then
    WORK="$WORK/host" sh "$HERE/tests/host-jq.sh" > "$WORK/host-tests.log" 2>&1 || { tail -40 "$WORK/host-tests.log"; exit 1; }
    grep -E "tests passed|checks passed" "$WORK/host-tests.log"
fi

# --- GFOpenRegex: regex.library and its link stub -----------------------------
if [ -z "${RX_SRC:-}" ]; then fetch_at "$WORK/deps/gfopenregex" "$RX_REPO" "$RX_COMMIT"; RX_SRC="$WORK/deps/gfopenregex"; fi
RXB="$WORK/gfopenregex"
if [ ! -f "$RXB/lib/libregex.a" ] || [ "$(cat "$RXB/.rev" 2>/dev/null)" != "$RX_COMMIT" ]; then
    rm -rf "$RXB"
    STOVE="$STOVE" OUT="$RXB" HOST_TESTS=0 sh "$RX_SRC/build-amiga.sh" > "$WORK/regex.log" 2>&1 || { tail -30 "$WORK/regex.log"; exit 1; }
    echo "$RX_COMMIT" > "$RXB/.rev"
fi

# --- fpuauto: libfpuauto.a and its link specs ---------------------------------
FPD="$WORK/deps/fpuauto"
if [ "$(git -C "$FPD" rev-parse HEAD 2>/dev/null)" != "$FPA_COMMIT" ]; then
    rm -rf "$FPD"; mkdir -p "$FPD"; git -C "$FPD" init -q
    git -C "$FPD" config core.sparseCheckout true
    echo "/common/fpuauto/" > "$FPD/.git/info/sparse-checkout"
    git -C "$FPD" fetch -q --depth 1 --filter=blob:none "$FPA_REPO" "$FPA_COMMIT"
    git -C "$FPD" checkout -q FETCH_HEAD
fi
FA="$FPD/common/fpuauto"; FAL="$WORK/fpuauto"
if [ ! -f "$FAL/libfpuauto.a" ] || [ "$(cat "$FAL/.rev" 2>/dev/null)" != "$FPA_COMMIT" ]; then
    rm -rf "$FAL"; mkdir -p "$FAL/obj"
    m68k-amigaos-gcc -m68040 -DFPUAUTO_COMMON -c "$FA/fpuauto.S" -o "$FAL/obj/fa_common.o"
    for n in $(sed 's/^--wrap=//' "$FA/fpuauto.wraps"); do
        m68k-amigaos-gcc -m68040 -DR_$n -c "$FA/fpuauto.S" -o "$FAL/obj/fa_$n.o"
    done
    m68k-amigaos-ar rcs "$FAL/libfpuauto.a" "$FAL"/obj/fa_*.o
    echo "$FPA_COMMIT" > "$FAL/.rev"
fi

# --- jq at its pinned commit, patches/, the generated files, the probe --------
SRC="$WORK/jq"
jq_source "$SRC"
jq_generate "$SRC" "GFOS: AmigaOS 3.2 m68k-amigaos 68040, regex.library (GFOpenRegex)"
LINK="m68k-amigaos-gcc -noixemul -m68040 -specs=$HERE/scripts/m68k-soft-m68040.specs -specs=$FA/fpuauto.specs -L$FAL"
PROBE_CC="$LINK" PROBE_BIGENDIAN=1 PROBE_GFOS=1 jq_probe "$SRC" "$WORK/jq-config.h"
echo "probe: $(grep -c '^#define HAVE_' "$WORK/jq-config.h") functions found"

# --- compile -------------------------------------------------------------------
LATE=""
if m68k-amigaos-gcc -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then LATE="-fno-late-combine-instructions"; fi
# no loops turned into memset/memcpy calls (GCC 16), address 0 is memory
CF="-noixemul -m68040 -O2 -fno-tree-loop-distribute-patterns -fno-delete-null-pointer-checks $LATE \
 -Werror=implicit-function-declaration -Werror=int-conversion -Wno-unused-parameter \
 -include $WORK/jq-config.h -I$HERE/amiga/include -I$RXB/include -I$HERE/amiga -I$SRC -I$SRC/src -I$SRC/vendor \
 -DJQ_AMIGA_VERSION=\\\"$JQ_VERSION\\\" -DJQ_AMIGA_DATE=\\\"$(date -u +%d.%m.%Y)\\\""
O="$WORK/obj"
# objects made against another jq or patch set go
[ "$(cat "$O/.src" 2>/dev/null)" = "$(cat "$SRC/.patched")" ] || { rm -rf "$O"; mkdir -p "$O"; cat "$SRC/.patched" > "$O/.src"; }
M="$O/Makefile"
{
    echo "CF = $CF"
    A=""
    for f in $JQ_SRCS; do
        o="$(echo "$f" | tr / _ | sed 's/\.c$/.o/')"; A="$A $o"
        printf '%s: %s\n\tm68k-amigaos-gcc $(CF) -MMD -c $< -o $@\n' "$o" "$SRC/$f"
    done
    for f in rx_names pthread_one amiga_paths amiga_compat amiga_env; do
        A="$A amiga_$f.o"
        printf 'amiga_%s.o: %s\n\tm68k-amigaos-gcc $(CF) -MMD -c $< -o $@\n' "$f" "$HERE/amiga/$f.c"
    done
    echo "OBJS =$A"
    echo "all: \$(OBJS)"
    echo "-include *.d"
} > "$M"
(cd "$O" && nice -n 19 make -k -j"$JOBS" all > make.log 2>&1) || { grep -E 'error' "$O/make.log" | head -40; tail -20 "$O/make.log"; exit 1; }
echo "compile: $(( $(date +%s) - t0 )) s"

# --- link: soft-float specs, then fpuauto's; regex.library's stub ---------------
# shellcheck disable=SC2046
$LINK -o "$O/jq.debug" $(cd "$O" && for f in $(sed -n 's/^OBJS =//p' Makefile); do echo "$O/$f"; done) \
    -L"$RXB/lib" -lregex -lm > "$O/link.log" 2>&1 || { head -60 "$O/link.log"; exit 1; }

# --- the GFOpenUp part: Parts/jq -------------------------------------------------
P="$OUT/Parts/jq"; rm -rf "$P"; mkdir -p "$P/C" "$P/Libs" "$P/Docs/jq"
m68k-amigaos-strip -o "$P/C/jq" "$O/jq.debug"
cp "$RXB/lib/regex.library" "$P/Libs/regex.library"
cp "$HERE/README.md" "$P/Docs/jq/ReadMe"
cp "$SRC/COPYING" "$P/Docs/jq/COPYING-jq"
cp "$SRC/jq.1.prebuilt" "$P/Docs/jq/jq.1"
cp "$RX_SRC/LICENSE" "$P/Docs/jq/LICENSE-GFOpenRegex"
sh "$HERE/scripts/write-part.sh" "$P" "$JQ_VERSION"
# jq's own test files, for jq --run-tests in a lab (not in the part)
rm -rf "$OUT/tests"; mkdir -p "$OUT/tests"
cp "$SRC"/tests/jq.test "$SRC"/tests/man.test "$SRC"/tests/base64.test "$SRC"/tests/uri.test "$OUT/tests/"
# jq.test's module tests read these (jq -L modules --run-tests jq.test)
cp -R "$SRC/tests/modules" "$OUT/tests/modules"
python3 "$HERE/tests/skip_cases.py" "$SRC/tests/onig.test" "$HERE/tests/onig-skips.txt" > "$OUT/tests/onig-gf.test"
ls -l "$P/C/jq" "$P/Libs/regex.library"
echo "part: $(grep -c '^copy\|^add' "$P/Part") files, $(du -sk "$P" | cut -f1) KB"
echo "fpuauto: $(m68k-amigaos-nm "$O/jq.debug" | grep -c ' T ___wrap_') wrapped routines"
echo "total: $(( $(date +%s) - t0 )) s"
