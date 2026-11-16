/* PC test of amiga/amiga_env.c's list: what jq's env and $ENV see on GFOS.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "amiga_env.h"

static int fails, checks;
static void ok(int c, const char *what) { checks++; if (!c) { fails++; printf("FAIL %s\n", what); } }

int main(void)
{
	env_reset();
	ok(environ && environ[0] == NULL, "empty list is a valid environ");
	ok(env_add("Workbench", 9, "47.3", 4) == 1, "local added");
	ok(env_add("Language", 8, "english", 7) == 1, "second added");
	ok(env_add("Workbench", 9, "40.1", 4) == 0, "global after local is not added");
	ok(env_add("Work", 4, "x", 1) == 1, "a name that prefixes another is its own");
	ok(env_add("", 0, "x", 1) == 0, "empty name skipped");
	ok(env_add("Empty", 5, "", 0) == 1, "empty value kept");
	ok(strcmp(environ[0], "Workbench=47.3") == 0, "local value wins");
	ok(strcmp(environ[1], "Language=english") == 0, "order kept");
	ok(strcmp(environ[2], "Work=x") == 0, "prefix name");
	ok(strcmp(environ[3], "Empty=") == 0, "empty value");
	ok(environ[4] == NULL, "NULL-terminated");
	for (int i = 0; i < 100; i++) { char n[16]; int l = sprintf(n, "V%d", i); env_add(n, l, n, l); }
	ok(strcmp(environ[103], "V99=V99") == 0 && environ[104] == NULL, "grows past its first block");
	env_reset();
	ok(environ[0] == NULL, "reset empties");
	printf("env: %d of %d checks passed\n", checks - fails, checks);
	return fails != 0;
}
