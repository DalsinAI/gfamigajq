/* PC test for amiga/amiga_paths.c and amiga/rx_names.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "libgen.h"
#include "rx_names.h"

static int fails;

static void dn(const char *in, const char *want_dir, const char *want_base)
{
	char a[256], b[256];
	strcpy(a, in); strcpy(b, in);
	const char *d = dirname(a), *n = basename(b);
	if (strcmp(d, want_dir) || strcmp(n, want_base)) {
		printf("FAIL %s: dirname %s (want %s), basename %s (want %s)\n", in, d, want_dir, n, want_base);
		fails++;
	}
}

static void names(const char *pat, int x, const char *want)
{
	char **n, got[256] = "";
	int c = rx_group_names(pat, strlen(pat), x, &n), i;
	for (i = 0; i < c; i++) {
		strcat(got, n[i] ? n[i] : "-");
		strcat(got, ",");
	}
	rx_names_free(n, c);
	if (strcmp(got, want)) { printf("FAIL names %s: %s (want %s)\n", pat, got, want); fails++; }
}

int main(void)
{
	dn("C:jq", "C:", "jq");
	dn("Work:jq/lib/main.jq", "Work:jq/lib", "main.jq");
	dn("Work:jq/", "Work:", "jq");
	dn("jq", "", "jq");
	dn("lib/jq", "lib", "jq");
	dn("/usr/bin/jq", "/usr/bin", "jq");
	dn("", "", "");
	names("(a)(?<x>b)", 0, "-,x,");
	names("(?:a)(?P<y>b)(?'z'c)", 0, "y,z,");
	names("(?=a)(?!b)(?#(c))[(]\\(d\\)(e)", 0, "-,");
	names("\\Q(a)\\E(b)", 0, "-,");
	names("[[:alpha:](](x)", 0, "-,");
	names("(a) # (b)\n(c)", 1, "-,-,");
	names("(a) # (b)\n(c)", 0, "-,-,-,");
	names("(?<n>(?<m>a))", 0, "n,m,");
	if (!fails) printf("paths and names: all checks passed\n");
	return fails != 0;
}
