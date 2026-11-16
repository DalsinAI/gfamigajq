/* The names of a Perl pattern's capture groups, in group order:
 * regex.library numbers (?<n>...), (?P<n>...) and (?'n'...) as usual but
 * keeps no names, and jq's match() reports each capture's name.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdlib.h>
#include <string.h>
#include "rx_names.h"

/* Reads pattern[0..len) as Perl syntax does for groups only: escapes,
 * bracket classes, \Q...\E, (?#...) comments and, with x, # comments are
 * skipped so their parentheses don't count. names[i] is group i+1's name
 * (malloc'd, NUL-ended) or 0. Returns the number of capture groups, or
 * -1 when out of memory. */
int rx_group_names(const char *p, size_t len, int extended, char ***names_out)
{
	char **names = 0;
	int n = 0, cap = 0;
	size_t i = 0;
	*names_out = 0;
	while (i < len) {
		char c = p[i];
		if (c == '\\') {
			if (i + 1 < len && p[i + 1] == 'Q') {      /* \Q...\E: all literal */
				i += 2;
				while (i < len && !(p[i] == '\\' && i + 1 < len && p[i + 1] == 'E')) i++;
				i += 2;
				continue;
			}
			i += 2;
			continue;
		}
		if (c == '[') {                                   /* a class: up to its ] */
			i++;
			if (i < len && p[i] == '^') i++;
			if (i < len && p[i] == ']') i++;              /* a leading ] is literal */
			while (i < len && p[i] != ']') {
				if (p[i] == '\\') i++;
				else if (p[i] == '[' && i + 1 < len && (p[i + 1] == ':' || p[i + 1] == '=' || p[i + 1] == '.')) {
					char k = p[i + 1];
					i += 2;
					while (i + 1 < len && !(p[i] == k && p[i + 1] == ']')) i++;
					i++;
				}
				i++;
			}
			i++;
			continue;
		}
		if (extended && c == '#') {                       /* x: comment to the line end */
			while (i < len && p[i] != '\n') i++;
			continue;
		}
		if (c != '(') { i++; continue; }
		/* a group: which kind? */
		{
			const char *name = 0;
			size_t nlen = 0;
			int capture = 1;
			if (i + 1 < len && p[i + 1] == '?') {
				size_t j = i + 2;
				capture = 0;
				if (j < len && p[j] == '#') {             /* (?#...) */
					while (j < len && p[j] != ')') j++;
					i = j + 1;
					continue;
				}
				if (j < len && p[j] == 'P' && j + 1 < len && p[j + 1] == '<') j++;
				if (j < len && (p[j] == '<' || p[j] == '\'') && j + 1 < len &&
				    p[j + 1] != '=' && p[j + 1] != '!') {
					char close = p[j] == '<' ? '>' : '\'';
					size_t s = j + 1;
					j = s;
					while (j < len && p[j] != close) j++;
					name = p + s;
					nlen = j - s;
					capture = 1;
				}
			}
			if (capture) {
				if (n == cap) {
					char **nn;
					cap = cap ? cap * 2 : 8;
					nn = realloc(names, cap * sizeof *names);
					if (!nn) { rx_names_free(names, n); return -1; }
					names = nn;
				}
				names[n] = 0;
				if (name) {
					names[n] = malloc(nlen + 1);
					if (!names[n]) { rx_names_free(names, n); return -1; }
					memcpy(names[n], name, nlen);
					names[n][nlen] = 0;
				}
				n++;
			}
			i++;
		}
	}
	*names_out = names;
	return n;
}

void rx_names_free(char **names, int n)
{
	int i;
	if (!names) return;
	for (i = 0; i < n; i++) free(names[i]);
	free(names);
}
