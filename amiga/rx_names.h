/* The names of a Perl pattern's capture groups (rx_names.c).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef RX_NAMES_H
#define RX_NAMES_H
#include <stddef.h>
int rx_group_names(const char *pattern, size_t len, int extended, char ***names);
void rx_names_free(char **names, int n);
#endif
