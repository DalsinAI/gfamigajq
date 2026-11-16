/* AmigaDOS dirname and basename (include/libgen.h). Both may change the
 * string they're given, as POSIX allows.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <string.h>
#include "libgen.h"

char *gfjq_dirname(char *path)
{
	static char dot[] = "";
	char *slash, *colon;
	size_t n;
	if (!path || !*path) return dot;         /* the current drawer is "" */
	n = strlen(path);
	while (n > 1 && path[n - 1] == '/') path[--n] = 0;
	slash = strrchr(path, '/');
	colon = strrchr(path, ':');
	if (slash && (!colon || slash > colon)) {
		if (slash == path) slash[1] = 0;     /* "/x": the parent, "/" */
		else *slash = 0;
		return path;
	}
	if (colon) {                             /* "C:jq" -> "C:" */
		colon[1] = 0;
		return path;
	}
	return dot;
}

char *gfjq_basename(char *path)
{
	static char dot[] = "";
	char *slash, *colon, *b;
	size_t n;
	if (!path || !*path) return dot;
	n = strlen(path);
	while (n > 1 && path[n - 1] == '/') path[--n] = 0;
	slash = strrchr(path, '/');
	colon = strrchr(path, ':');
	b = path;
	if (slash && (!colon || slash > colon)) b = slash + 1;
	else if (colon) b = colon + 1;
	return b;
}
