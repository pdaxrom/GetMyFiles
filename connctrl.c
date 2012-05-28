#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef _WIN32
#include <sys/select.h>
#include <pthread.h>
#else
#include <windows.h>
#ifdef ENABLE_PTHREADS
#include <pthread.h>
#endif
#endif

#include "connctrl.h"

#if !defined(_WIN32) || defined(ENABLE_PTHREADS)
#define mutex_t		pthread_mutex_t
#define mutex_init(a,b)	pthread_mutex_init(a,b)
#define mutex_lock(a)	pthread_mutex_lock(a)
#define mutex_unlock(a)	pthread_mutex_unlock(a)
#define mutex_destroy(a) pthread_mutex_destroy(a)
#else
#define mutex_t		CRITICAL_SECTION
#define mutex_init(a,b)	InitializeCriticalSection(a)
#define mutex_lock(a)	EnterCriticalSection(a)
#define mutex_unlock(a) LeaveCriticalSection(a)
#define mutex_destroy(a) DeleteCriticalSection(a)
#endif

static mutex_t mutex_cnt[CONN_TYPE_MAX];
static int cnt[CONN_TYPE_MAX];
static int max_cnt[CONN_TYPE_MAX];

void conn_counter_init(int type, int _max_cnt)
{
    max_cnt[type] = _max_cnt;
    cnt[type] = 0;
    mutex_init(&mutex_cnt[type], NULL);
}

void conn_counter_fini(int type)
{
    mutex_destroy(&mutex_cnt[type]);
}

void conn_counter_inc(int type)
{
    mutex_lock(&mutex_cnt[type]);
    cnt[type]++;
    mutex_unlock(&mutex_cnt[type]);
}

void conn_counter_dec(int type)
{
    mutex_lock(&mutex_cnt[type]);
    cnt[type]--;
    mutex_unlock(&mutex_cnt[type]);
}

int conn_counter_limit(int type)
{
    if (max_cnt[type] == 0)
	return 0;

    mutex_lock(&mutex_cnt[type]);
    int c = cnt[type];
    mutex_unlock(&mutex_cnt[type]);

    if (c >= max_cnt[type])
	return 1;

    return 0;
}
