#!/usr/bin/env python3
"""Writes a jq --run-tests file without the cases whose program line is
listed in the skips file (one program line per entry; lines starting with
'#' give the reasons).
   skip_cases.py TESTFILE SKIPS > OUT
MIT, Copyright (c) 2026 AmiCradle Team."""
import sys


def blocks(text):
    """jq's test file: cases are runs of lines between blank lines."""
    cur = []
    for line in text.split("\n"):
        if line.strip() == "":
            if cur:
                yield cur
                cur = []
        else:
            cur.append(line)
    if cur:
        yield cur


def program(block):
    lines = [l for l in block if not l.startswith("#")]
    if lines and lines[0].startswith("%%"):
        lines = lines[1:]
    return lines[0] if lines else None


def main():
    test, skips = sys.argv[1], sys.argv[2]
    skip = {l.rstrip("\n") for l in open(skips, encoding="utf-8")
            if l.strip() and not l.startswith("#")}
    used, out = set(), []
    for b in blocks(open(test, encoding="utf-8").read()):
        p = program(b)
        if p in skip:
            used.add(p)
            continue
        out.append("\n".join(b))
    missing = sorted(skip - used)
    for s in missing:
        print(f"skip_cases.py: no case {s!r} in {test}", file=sys.stderr)
    if missing:
        sys.exit(1)
    print("\n\n".join(out))


main()
