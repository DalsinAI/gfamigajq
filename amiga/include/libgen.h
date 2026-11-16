/* dirname and basename that know AmigaDOS paths (a volume or assign ends
 * at its colon: "Work:jq/lib" -> "Work:jq", "C:jq" -> "C:"), for jq's
 * $ORIGIN and module paths. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFJQ_LIBGEN_H
#define GFJQ_LIBGEN_H
char *gfjq_dirname(char *path);
char *gfjq_basename(char *path);
/* jq's module paths ("dir" + "/" + "./../lib/jq/e.jq", Unix joins) as an
 * AmigaDOS name into out (n bytes): no separator after a volume's colon,
 * "." and empty parts dropped, ".." the parent ("Work:a/b/../c" ->
 * "Work:a/c", "x/../../y" -> "/y"). The length, or 0 when out is too small. */
#include <stddef.h>
size_t gfjq_amiga_path(const char *dir, const char *rel, char *out, size_t n);
#define dirname gfjq_dirname
#define basename gfjq_basename
#endif
