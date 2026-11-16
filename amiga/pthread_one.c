/* pthread.h's calls for one task (see pthread.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <errno.h>
#include "pthread.h"

#define GFJQ_KEYS 16
static void *slot[GFJQ_KEYS];
static unsigned int nkeys;

int pthread_once(pthread_once_t *once, void (*fn)(void))
{
	if (!*once) {
		*once = 1;
		fn();
	}
	return 0;
}

int pthread_key_create(pthread_key_t *key, void (*dtor)(void *))
{
	if (nkeys >= GFJQ_KEYS) return EAGAIN;
	(void)dtor;  /* jq frees its contexts itself, at exit */
	slot[nkeys] = NULL;
	*key = nkeys++;
	return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
	return key < nkeys ? slot[key] : NULL;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
	if (key >= nkeys) return EINVAL;
	slot[key] = (void *)value;
	return 0;
}
