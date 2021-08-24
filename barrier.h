#ifndef BARRIER_H
#define BARRIER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int *count, nThreads, iter;
} barrier_t;

int barrier_init(barrier_t* barrier, int n);
int barrier_destroy(barrier_t* barrier);
int barrier_wait(barrier_t* barrier, void (*f)(void*), void* fArg);

#endif
