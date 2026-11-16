# What build-amiga.sh (the stove) and tests/host-jq.sh (the PC) share:
# fetching jq, its generated files and the probe that writes jq-config.h,
# the -D lines jq's configure would give. Sourced; needs HERE (the repo)
# and jq.lock's settings.
# MIT, Copyright (c) 2026 AmiCradle Team.

# jq's library and program, as upstream's Makefile.am (LIBJQ_SRC, jq_SOURCES)
JQ_SRCS="src/builtin.c src/bytecode.c src/compile.c src/execute.c src/jq_test.c
 src/jv.c src/jv_alloc.c src/jv_aux.c src/jv_dtoa.c src/jv_file.c src/jv_parse.c
 src/jv_print.c src/jv_unicode.c src/linker.c src/locfile.c src/util.c
 src/jv_dtoa_tsd.c src/lexer.c src/parser.c
 vendor/decNumber/decContext.c vendor/decNumber/decNumber.c src/main.c"

# a repository at one commit, fetched once: fetch_at DIR URL COMMIT
fetch_at() {
    if [ "$(git -C "$1" rev-parse HEAD 2>/dev/null)" != "$3" ]; then
        rm -rf "${1:?}"; mkdir -p "$1"
        git -C "$1" init -q
        git -C "$1" fetch -q --depth 1 "$2" "$3"
        git -C "$1" checkout -q FETCH_HEAD
        [ "$(git -C "$1" rev-parse HEAD)" = "$3" ] || { echo "$1: commit mismatch"; exit 1; }
    fi
}

# jq at JQ_COMMIT with patches/ applied, into $1 (redone when either changes)
jq_source() {
    PSUM=$(cat "$HERE"/patches/*.patch | sha256sum | cut -c1-16)
    if [ "$(cat "$1/.patched" 2>/dev/null)" != "$JQ_COMMIT-$PSUM" ]; then
        rm -rf "$1"
        fetch_at "$1" "$JQ_GIT" "$JQ_COMMIT"
        for p in "$HERE"/patches/*.patch; do
            patch -s -d "$1" -p1 < "$p" || { echo "patch failed: $p"; exit 1; }
        done
        echo "$JQ_COMMIT-$PSUM" > "$1/.patched"
    fi
}

# the files upstream's Makefile makes: src/builtin.inc (builtin.jq as
# bytes, the same od/sed), src/version.h, src/config_opts.inc
jq_generate() {
    (cd "$1" && od -v -A n -t o1 -- src/builtin.jq | \
        sed -e 's/$/ /' -e 's/\([0123456789]\) /\1, /g' -e 's/ $//' \
            -e 's/ 0/  0/g' -e 's/ \([123456789]\)/ 0\1/g' > src/builtin.inc)
    printf '#define JQ_VERSION "%s"\n' "$JQ_VERSION" > "$1/src/version.h"
    printf '#define JQ_CONFIG "%s"\n' "$2" > "$1/src/config_opts.inc"
}

# jq-config.h into $2: configure's answers, found by linking each function
# as autoconf does, and only where a header declares it
# ($PROBE_CC: the compile and link command).
# $1 is jq's tree (its configure.ac lists the maths functions).
# PROBE_BIGENDIAN=1: the 68k (jv_dtoa's IEEE_MC68k), else IEEE_8087.
# PROBE_GFOS=1: environ is amiga/amiga_env.c's (gfjq_environ) in every file.
jq_probe() {
    t=$(mktemp -d)
    {
        echo "/* jq-config.h: written by scripts/jq-common.sh jq_probe */"
        echo "#ifndef _GNU_SOURCE"
        echo "#define _GNU_SOURCE 1   /* configure's AC_USE_SYSTEM_EXTENSIONS */"
        echo "#endif"
        echo "#define USE_DECNUM 1"
        echo "#define HAVE_GFOPENREGEX 1"
        echo "#define HAVE_PTHREAD_KEY_CREATE 1"
        echo "#define HAVE_PTHREAD_ONCE 1"
        echo "#define HAVE_ATEXIT 1"
        if [ "${PROBE_GFOS:-}" = 1 ]; then
            # GFOS: every file's environ is amiga/amiga_env.c's list (Shell
            # locals and ENV: globals). libnix's headers name another list
            # environ (locals only), so theirs is taken first and replaced.
            echo "#include <stdlib.h>"
            echo "#include <unistd.h>"
            echo "#undef environ"
            echo "#define environ gfjq_environ"
            echo "extern char **environ;"
        fi
        echo "#ifndef PATH_MAX"
        echo "#include <limits.h>"
        echo "#endif"
        echo "#ifndef PATH_MAX"
        echo "#define PATH_MAX 1024"
        echo "#endif"
        for f in $(sed -n 's/^AC_CHECK_MATH_FUNC(\([a-z0-9_]*\)).*/\1/p' "$1/configure.ac") \
                 memmem isatty strptime strftime setenv timegm gmtime_r gmtime \
                 localtime_r localtime gettimeofday setlocale realpath; do
            # found by the link AND declared by the C library's headers: a
            # function in the library but in no header (libnix's scalb and
            # significand) would be an implicit declaration, so jq's own
            # fallback is taken instead
            printf '%s\n' '#define _GNU_SOURCE 1' '#include <math.h>' '#include <stdlib.h>' \
                '#include <string.h>' '#include <stdio.h>' '#include <time.h>' '#include <unistd.h>' \
                '#include <locale.h>' '#include <limits.h>' '#include <sys/time.h>' > "$t/p.c"
            printf 'int main(void) { void (*volatile p)(void) = (void (*)(void))%s; return p != 0; }\n' "$f" >> "$t/p.c"
            # shellcheck disable=SC2086
            if $PROBE_CC -o "$t/p" "$t/p.c" -lm > /dev/null 2>&1; then
                echo "#define HAVE_$(echo "$f" | tr '[:lower:]' '[:upper:]') 1"
            fi
        done
        printf '#include <time.h>\nint main(void) { struct tm t; t.tm_gmtoff = 0; return (int)t.tm_gmtoff; }\n' > "$t/p.c"
        # shellcheck disable=SC2086
        if $PROBE_CC -o "$t/p" "$t/p.c" > /dev/null 2>&1; then echo "#define HAVE_TM_TM_GMT_OFF 1"; fi
        if [ "${PROBE_BIGENDIAN:-}" = 1 ]; then echo "#define IEEE_MC68k 1"; else echo "#define IEEE_8087 1"; fi
    } > "$2"
    rm -rf "$t"
}
