# GFAmigaJQ

jq 1.8.1, the command-line JSON processor, for GFOS (AmigaOS 3.2.3 with
GFOpenUp) on a 68040 or 68060, installed as a GFOpenUp part: `C:jq`.

    jq '.ports[] | select(.state == "released") | .title' ports.json

Status: built for the PC from the same sources and passing jq's own tests
(see Tests); the Amiga build is made by the stove, not yet run in a lab.

## What you need

- GFOS: AmigaOS 3.2 with GFOpenUp, a 68040 or 68060. Soft float: the FPU
  is used when there is one (fpuauto), so an LC040 runs it too.
- Nothing else: the part brings `LIBS:regex.library` (GFOpenRegex) for
  `test`, `match`, `capture`, `sub`, `gsub`, `scan` and `splits`.

Install with `GFOpenUpTool INSTALL Parts/jq`. The manual is
`SYS:Docs/jq/jq.1` (the manual page; https://jqlang.org/manual/ is the
same text).

## On AmigaOS

- Regular expressions are Perl syntax on GFOpenRegex in place of
  Oniguruma. Modifiers as upstream's (`g i x m s p n`; `l` is taken but
  matching stays leftmost-first). Not offered: look-behind, possessive
  and atomic groups, `\p{..}` (they give "Regex failure").
- Module paths (`-L DIR`, `import`) are AmigaDOS paths (`Work:jq/lib`).
  With no `HOME` variable, `~/.jq` is not searched.
- jq runs in one task: the thread calls its library makes are served by
  a one-task stand-in (amiga/include/pthread.h).
- 256 KB of stack is set in the program, so the Shell's 4 KB is enough.

## Building

    STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out sh build-amiga.sh

or in Cradle, stove_build with the os32-gcc16-open6 stove, target `.`.
jq, GFOpenRegex and fpuauto are fetched at the commits `jq.lock` pins.
`OUT` gets `Parts/jq` (the part) and `tests/` (jq's test files, for
`jq --run-tests` in a lab).

## Tests

`make -C tests test` on a PC: the Part file, the AmigaDOS `dirname` and
`basename`, the capture-name reader, then `tests/host-jq.sh`, which
builds jq for the PC from the same patched sources, probe and stand-ins,
with `match()` on GFOpenRegex's own core, and runs jq's own test files:
jq.test (522), man.test (231, every example in the manual), base64.test,
uri.test and onig.test (45 of 47 regex cases; `tests/onig-skips.txt` has
the two regex.library can't pass yet and why).

## What was changed

- `patches/0001`: `src/builtin.c` takes `amiga/jq_gfregex.inc` for
  `match()` when `HAVE_GFOPENREGEX` is defined.
- `amiga/jq_gfregex.inc`: `match()` on `regncomp`/`regnexec` (Perl syntax,
  UTF-8); `amiga/rx_names.c` reads the capture groups' names from the
  pattern, as regex.library numbers named groups but doesn't keep names.
- `amiga/include/pthread.h`, `amiga/pthread_one.c`: keys, once and a
  mutex for one task.
- `amiga/include/libgen.h`, `amiga/amiga_paths.c`: `dirname`/`basename`
  that know `Volume:` paths.
- `amiga/amiga_compat.c`: stack, `$VER`, `realpath()` on AmigaDOS locks
  (when libnix has none).
- `scripts/jq-common.sh`: the generated files and a configure-style probe
  (each function linked, as autoconf does) giving `jq-config.h`.

Designed, tested and delivered by The AmiCradle Team.
