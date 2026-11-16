/* environ for jq on AmigaOS: the "NAME=value" list jq's env and $ENV read.
 * libnix has no environ, so it is built once at start: the Shell's local
 * variables first, then the global ones (top-level files in ENV:), a name
 * set both ways appearing once with its local value, as GetVar() gives it.
 * env_add() is the part the PC tests check (tests/env_test.c).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdlib.h>
#include <string.h>
#include "amiga_env.h"

char **environ;
static char *no_vars[1];
static size_t env_n, env_cap;

int env_add(const char *name, size_t nlen, const char *val, size_t vlen)
{
	size_t i;
	char *s, **grown;
	if (nlen == 0) return 0;
	for (i = 0; i < env_n; i++)
		if (strncmp(environ[i], name, nlen) == 0 && environ[i][nlen] == '=')
			return 0;       /* the first (local) one stays */
	if (env_n + 1 >= env_cap) {
		size_t cap = env_cap ? env_cap * 2 : 32;
		grown = realloc(env_cap ? environ : NULL, cap * sizeof *grown);
		if (!grown) return -1;
		environ = grown; env_cap = cap;
	}
	s = malloc(nlen + vlen + 2);
	if (!s) return -1;
	memcpy(s, name, nlen); s[nlen] = '=';
	memcpy(s + nlen + 1, val, vlen); s[nlen + 1 + vlen] = 0;
	environ[env_n++] = s;
	environ[env_n] = NULL;
	return 1;
}

void env_reset(void)
{
	size_t i;
	for (i = 0; i < env_n; i++) free(environ[i]);
	if (env_cap) free(environ);
	env_n = env_cap = 0;
	environ = no_vars;
}

#ifdef __amigaos__
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/var.h>
#include <proto/exec.h>
#include <proto/dos.h>

static void env_globals(void)
{
	struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
	BPTR lock = fib ? Lock((CONST_STRPTR)"ENV:", SHARED_LOCK) : 0;
	if (lock && Examine(lock, fib)) {
		while (ExNext(lock, fib)) {
			char *buf;
			LONG n;
			if (fib->fib_DirEntryType >= 0) continue;   /* a drawer (prefs), not a variable */
			buf = malloc(fib->fib_Size + 1);
			if (!buf) break;
			n = GetVar((CONST_STRPTR)fib->fib_FileName, (STRPTR)buf, fib->fib_Size + 1,
			           GVF_GLOBAL_ONLY | GVF_BINARY_VAR | GVF_DONT_NULL_TERM);
			if (n >= 0)
				env_add(fib->fib_FileName, strlen(fib->fib_FileName), buf, (size_t)n);
			free(buf);
		}
	}
	if (lock) UnLock(lock);
	if (fib) FreeDosObject(DOS_FIB, fib);
}

static void __attribute__((constructor)) env_start(void)
{
	struct Process *me = (struct Process *)FindTask(NULL);
	struct LocalVar *lv;
	environ = no_vars;
	if (me->pr_Task.tc_Node.ln_Type == NT_PROCESS) {
		for (lv = (struct LocalVar *)me->pr_LocalVars.mlh_Head;
		     lv->lv_Node.ln_Succ; lv = (struct LocalVar *)lv->lv_Node.ln_Succ)
			if (lv->lv_Node.ln_Type == LV_VAR && !(lv->lv_Flags & GVF_BINARY_VAR) && lv->lv_Node.ln_Name)
				env_add(lv->lv_Node.ln_Name, strlen(lv->lv_Node.ln_Name),
				        (const char *)lv->lv_Value, lv->lv_Len);
	}
	env_globals();
}
#endif
