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

size_t gfjq_amiga_path(const char *dir, const char *rel, char *out, size_t n)
{
	char buf[1024], *seg[128], *t, *vol_end;
	int ns = 0, i, parent = 0, first = 1;
	size_t len = 0, l;
	if (!dir) dir = "";
	if (!rel) rel = "";
	if (strlen(dir) + strlen(rel) + 2 > sizeof buf) return 0;
	strcpy(buf, dir); strcat(buf, "/"); strcat(buf, rel);
	vol_end = strchr(buf, ':');
	if (vol_end) {                       /* "Work:" stays as it is */
		l = (size_t)(vol_end - buf) + 1;
		if (l >= n) return 0;
		memcpy(out, buf, l); len = l;
		t = vol_end + 1;
	} else t = buf;
	for (t = strtok(t, "/"); t; t = strtok(NULL, "/")) {
		if (!strcmp(t, ".")) continue;
		if (!strcmp(t, "..") && ns > 0 && strcmp(seg[ns - 1], "..")) { ns--; continue; }
		if (ns == (int)(sizeof seg / sizeof seg[0])) return 0;
		seg[ns++] = t;
	}
	for (i = 0; i < ns; i++) {
		int up = !strcmp(seg[i], "..");
		const char *s = up ? "/" : seg[i];
		l = strlen(s);
		if (!up && !first && !parent) {      /* the separator between two names */
			if (len + 1 >= n) return 0;
			out[len++] = '/';
		}
		if (len + l >= n) return 0;
		memcpy(out + len, s, l); len += l;
		parent = up; first = 0;
	}
	out[len] = 0;
	return len;
}
