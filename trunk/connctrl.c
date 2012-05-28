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

static mutex_t mutex_cnt;
static int cnt = 0;
static int max_cnt = 0;

void conn_counter_init(int _max_cnt)
{
    max_cnt = _max_cnt;
    mutex_init(&mutex_cnt, NULL);
}

void conn_counter_fini(void)
{
    mutex_destroy(&mutex_cnt);
}

void conn_counter_inc(void)
{
    mutex_lock(&mutex_cnt);
    cnt++;
    mutex_unlock(&mutex_cnt);
}

void conn_counter_dec(void)
{
    mutex_lock(&mutex_cnt);
    cnt--;
    mutex_unlock(&mutex_cnt);
}

int conn_counter_limit(void)
{
    if (max_cnt == 0)
	return 0;

    mutex_lock(&mutex_cnt);
    int c = cnt;
    mutex_unlock(&mutex_cnt);

    if (c >= max_cnt)
	return 1;

    return 0;
}
