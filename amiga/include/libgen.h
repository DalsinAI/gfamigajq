/* dirname and basename that know AmigaDOS paths (a volume or assign ends
 * at its colon: "Work:jq/lib" -> "Work:jq", "C:jq" -> "C:"), for jq's
 * $ORIGIN and module paths. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFJQ_LIBGEN_H
#define GFJQ_LIBGEN_H
char *gfjq_dirname(char *path);
char *gfjq_basename(char *path);
#define dirname gfjq_dirname
#define basename gfjq_basename
#endif
