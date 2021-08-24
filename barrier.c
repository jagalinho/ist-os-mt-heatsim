#include "barrier.h"

#define other(x) x? 0 :1

int barrier_init(barrier_t* barrier, int n) {
	if (pthread_mutex_init(&(barrier->mutex), NULL)) {
		fprintf(stderr, "Erro ao inicializar mutex.\n");
		return -1;
	}
	
	if (pthread_cond_init(&(barrier->cond), NULL)) {
		fprintf(stderr, "Erro ao inicializar variável de condição.\n");
		return -1;
	}
	
	barrier->count = (int*)malloc(2*sizeof(int));
	
	if (barrier->count == NULL) {
		fprintf(stderr, "\nErro ao criar array.\n");
		return -1;
	}
	
	barrier->count[0] = n;
	
	barrier->nThreads = n;
	
	barrier->iter = 0;
	
	return 0;
}

int barrier_destroy(barrier_t* barrier) {
	if (pthread_mutex_destroy(&(barrier->mutex))) {
    	fprintf(stderr, "Erro ao destruir mutex.\n");
    	return -1;
  	}
  	
  	if (pthread_cond_destroy(&(barrier->cond))) {
    	fprintf(stderr, "Erro ao destruir variável de condição.\n");
    	return -1;
  	}
  	
  	free(barrier->count);
  	
  	return 0;
}

int barrier_wait(barrier_t* barrier, void (*f)(void*), void* fArg) {
	if (pthread_mutex_lock(&(barrier->mutex))) {
    	fprintf(stderr, "Erro ao bloquear mutex.\n");
    	return -1;
  	}
  	
  	int iter = barrier->iter;
  	
  	barrier->count[iter]--;
  	
  	if (barrier->count[iter]) {
  		while (barrier->count[iter])
  			if (pthread_cond_wait(&(barrier->cond), &(barrier->mutex))) {
        		fprintf(stderr, "Erro ao esperar por variável de condição.\n");
        		return -1;
      		}
    } else {
     	(*f)(fArg);
     	barrier->iter = other(barrier->iter);
		barrier->count[barrier->iter] = barrier->nThreads;
      	if (pthread_cond_broadcast(&(barrier->cond))) {
    		fprintf(stderr, "Erro ao desbloquear variável de condição.\n");
    		return -1;
  		}
      }
    
    if (pthread_mutex_unlock(&(barrier->mutex))) {
    	fprintf(stderr, "Erro ao desbloquear mutex.\n");
    	return -1;
  	}
  	
  	return 0;
}
