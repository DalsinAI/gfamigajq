/* The few POSIX thread calls jq's library makes (keys for its per-thread
 * number contexts, once, a mutex), for one Amiga task: jq runs in one
 * task, so a key is a slot, once is a flag and a mutex does nothing.
 * Key destructors aren't kept: jq frees its contexts at exit itself.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFJQ_PTHREAD_H
#define GFJQ_PTHREAD_H
#include <stddef.h>

/* our own names, so the PC test's C library (which has its own) doesn't clash */
#define pthread_once_t gfjq_once_t
#define pthread_key_t gfjq_key_t
#define pthread_mutex_t gfjq_mutex_t
#define pthread_once gfjq_once
#define pthread_key_create gfjq_key_create
#define pthread_getspecific gfjq_getspecific
#define pthread_setspecific gfjq_setspecific
#define pthread_mutex_lock gfjq_mutex_lock
#define pthread_mutex_unlock gfjq_mutex_unlock
#define pthread_mutex_init gfjq_mutex_init
#define pthread_mutex_destroy gfjq_mutex_destroy
#undef PTHREAD_ONCE_INIT
#undef PTHREAD_MUTEX_INITIALIZER

typedef int pthread_once_t;
typedef unsigned int pthread_key_t;
typedef int pthread_mutex_t;
#define PTHREAD_ONCE_INIT 0
#define PTHREAD_MUTEX_INITIALIZER 0

int pthread_once(pthread_once_t *once, void (*fn)(void));
int pthread_key_create(pthread_key_t *key, void (*dtor)(void *));
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
static inline int pthread_mutex_lock(pthread_mutex_t *m) { (void)m; return 0; }
static inline int pthread_mutex_unlock(pthread_mutex_t *m) { (void)m; return 0; }
static inline int pthread_mutex_init(pthread_mutex_t *m, const void *a) { (void)a; *m = 0; return 0; }
static inline int pthread_mutex_destroy(pthread_mutex_t *m) { (void)m; return 0; }

#endif
