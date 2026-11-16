/* jq on AmigaOS: the Shell's 4 KB stack is too small for jq's parser and
 * its interpreter, the $VER string, and realpath() on AmigaDOS locks
 * (libnix has none). MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <proto/dos.h>

unsigned long __stack = 256 * 1024;
static const char version[] __attribute__((used)) = "$VER: jq " JQ_AMIGA_VERSION " (" JQ_AMIGA_DATE ") GFOS";

#ifndef HAVE_REALPATH
/* The full name of an object that exists ("Work:jq/lib"), or NULL. */
char *realpath(const char *path, char *resolved)
{
	BPTR lock;
	char *buf = resolved;
	if (!path) return NULL;
	lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
	if (!lock) return NULL;
	if (!buf) buf = malloc(PATH_MAX);
	if (buf && !NameFromLock(lock, (STRPTR)buf, PATH_MAX)) {
		if (buf != resolved) free(buf);
		buf = NULL;
	}
	UnLock(lock);
	return buf;
}
#endif
