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

static void ap(const char *dir, const char *rel, const char *want)
{
	char out[256];
	size_t n = gfjq_amiga_path(dir, rel, out, sizeof out);
	if (!n && *want) { printf("FAIL path %s + %s: too long\n", dir, rel); fails++; return; }
	if (strcmp(out, want)) { printf("FAIL path %s + %s: %s (want %s)\n", dir, rel, out, want); fails++; }
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
	ap("System:T/modules", "a.jq", "System:T/modules/a.jq");
	ap("Work:", "a.jq", "Work:a.jq");
	ap("Work:", "lib/jq/main.jq", "Work:lib/jq/main.jq");
	ap("System:T/modules/c", ".//d.jq", "System:T/modules/c/d.jq");
	ap("System:T/modules/c/./", "../lib/jq/e.jq", "System:T/modules/lib/jq/e.jq");
	ap("System:T/modules/c", "./../lib/jq/e/e.jq", "System:T/modules/lib/jq/e/e.jq");
	ap("C:", "../lib/jq", "C:/lib/jq");
	ap(".", "a.jq", "a.jq");
	ap("", "a.jq", "a.jq");
	ap("modules", "a.jq", "modules/a.jq");
	ap("x", "../../y.jq", "/y.jq");
	ap("x", "../../../y.jq", "//y.jq");
	ap("Work:a/b", "../c", "Work:a/c");
	ap("RAM:", "", "RAM:");
	{ char small[8]; if (gfjq_amiga_path("Work:", "longname.jq", small, sizeof small) != 0) { printf("FAIL path: too small not refused\n"); fails++; } }
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
