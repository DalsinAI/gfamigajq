/* environ for jq on AmigaOS (amiga_env.c). MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFJQ_AMIGA_ENV_H
#define GFJQ_AMIGA_ENV_H
#include <stddef.h>
extern char **environ;
/* adds "name=value" unless name is already there: 1 added, 0 already (or empty name), -1 no memory */
int env_add(const char *name, size_t nlen, const char *val, size_t vlen);
/* empties the list (environ stays a valid empty list) */
void env_reset(void);
#endif
