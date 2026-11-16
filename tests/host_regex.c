/* The PC tests' regex.library: GFOpenRegex's own core (src/) behind the
 * calls libregex.a gives on the Amiga, so jq's match() runs the same code
 * on the PC as through regex.library. MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdlib.h>
#include <regex.h>
#include "rx_core.h"
#include "rx_api.h"

static void *a(struct rx_env *e, rx_u32 n) { (void)e; return malloc(n); }
static void f(struct rx_env *e, void *p) { (void)e; free(p); }
static struct rx_env env = { a, f, 0 };

int regcomp(regex_t *p, const char *s, int c) { return (int)rx_api_comp(&env, p, s, c); }
int regncomp(regex_t *p, const char *s, size_t n, int c) { return (int)rx_api_ncomp(&env, p, s, n, c); }
int regexec(const regex_t *p, const char *s, size_t n, regmatch_t m[], int e)
{ return (int)rx_api_exec(&env, p, s, n, m, e); }
int regnexec(const regex_t *p, const char *s, size_t l, size_t n, regmatch_t m[], int e)
{ return (int)rx_api_nexec(&env, p, s, l, n, m, e); }
size_t regerror(int e, const regex_t *p, char *b, size_t n) { return rx_api_error(e, p, b, n); }
void regfree(regex_t *p) { rx_api_free(&env, p); }
