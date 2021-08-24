#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>

#include "matrix2d.h"
#include "barrier.h"

int iter, trab, columns;
double maxD, currMaxD, *threadCurrMaxD;
DoubleMatrix2D *matrix, *matrixAux;
barrier_t barrier;

typedef struct {
	int id, firstLine, lastLine;
} simulMatrix_args;

void endIter_simulMatrix(void *args) {
	DoubleMatrix2D *tmp;
	
	tmp = matrix;
    matrix = matrixAux;
    matrixAux = tmp;
	
	double max = threadCurrMaxD[0];
	int i;
	
	for (i=1; i<trab; i++)
		if (threadCurrMaxD[i] > max)
			max = threadCurrMaxD[i];
	
	currMaxD = max;
	
	iter--;
}

void* simulMatrix(void *args) {
	simulMatrix_args arg = *(simulMatrix_args*)args;
	
	int i, j;
	double diff;
	threadCurrMaxD[arg.id] = maxD + 1;
	
	while (iter && (currMaxD > maxD)) {
		threadCurrMaxD[arg.id] = 0;
		
		for (i=arg.firstLine; i<=arg.lastLine; i++)
      		for (j=1; j<columns-1; j++) {
        		dm2dSetEntry(matrixAux, i, j, (dm2dGetEntry(matrix, i-1, j) + dm2dGetEntry(matrix, i+1, j) + dm2dGetEntry(matrix, i, j-1) + dm2dGetEntry(matrix, i, j+1)) / 4.0);
        		diff = fabs(dm2dGetEntry(matrixAux, i, j) - dm2dGetEntry(matrix, i, j));
        		if (diff > threadCurrMaxD[arg.id])
        			threadCurrMaxD[arg.id] = diff;
        	}
        
        if (barrier_wait(&barrier, endIter_simulMatrix, NULL)) {
        	fprintf(stderr, "Erro: Nao foi possivel esperar por barriera.\n");
    		exit(EXIT_FAILURE);
        }
	}
	
	return NULL;
}

int parse_integer_or_exit(char const *str, char const *name) {
  int value;
 
  if (sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "Erro no argumento \"%s\".\n", name);
    exit(1);
  }
  
  return value;
}

double parse_double_or_exit(char const *str, char const *name) {
  double value;

  if (sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "Erro no argumento \"%s\".\n", name);
    exit(1);
  }
  
  return value;
}

int main(int argc, char** argv) {
  	if (argc != 9) {
    	fprintf(stderr, "Erro: Numero invalido de argumentos.\n");
    	return -1;
  	}
	
  	int N = parse_integer_or_exit(argv[1], "N");
  	double tEsq = parse_double_or_exit(argv[2], "tEsq");
  	double tSup = parse_double_or_exit(argv[3], "tSup");
  	double tDir = parse_double_or_exit(argv[4], "tDir");
  	double tInf = parse_double_or_exit(argv[5], "tInf");
  	iter = parse_integer_or_exit(argv[6], "iter");
  	maxD = parse_double_or_exit(argv[7], "maxD");
  	trab = parse_integer_or_exit(argv[8], "trab");
	
  	fprintf(stderr, "Argumentos: N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d maxD=%.4f trab=%d\n", N, tEsq, tSup, tDir, tInf, iter, maxD, trab);
	
  	if (N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 || trab < 1 || N%trab != 0 || maxD < 0) {
    	fprintf(stderr, "Erro: Argumentos invalidos.\n");
    	return -1;
  	}
	
  	matrix = dm2dNew(N+2, N+2);
  	matrixAux = dm2dNew(N+2, N+2);
	
  	if (matrix == NULL || matrixAux == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para as matrizes.\n");
    	return -1;
  	}
	
  	dm2dSetLineTo(matrix, 0, tSup);
  	dm2dSetLineTo(matrix, N+1, tInf);
  	dm2dSetColumnTo(matrix, 0, tEsq);
  	dm2dSetColumnTo(matrix, N+1, tDir);
  	
  	dm2dCopy(matrixAux, matrix);
  	
  	pthread_t *tid = (pthread_t*)malloc(trab*sizeof(pthread_t));
  	
  	if (tid == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para as thread id.\n");
    	return -1;
  	}
  	
  	if (barrier_init(&barrier, trab)) {
  		fprintf(stderr, "Erro: Nao foi possivel inicializar barriera.\n");
    	return -1;
  	}
  	
  	simulMatrix_args *args = (simulMatrix_args*)malloc(trab*sizeof(simulMatrix_args));
  	
  	if (args == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para os args.\n");
    	return -1;
  	}
  	
  	threadCurrMaxD = (double*)malloc(trab*sizeof(double));
  	
  	if (threadCurrMaxD == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para as threadCurrMaxD.\n");
    	return -1;
  	}
	
  	columns = N + 2;
  	currMaxD = maxD + 1;
  	
  	int i;
  	
  	for (i=0; i<trab; i++) {
  		args[i].id = i;
  		args[i].firstLine = ((N/trab)*i)+1;
  		args[i].lastLine = (N/trab)*(i+1);
  		
  		if (pthread_create(&tid[i], NULL, simulMatrix, &args[i])) {
  			fprintf(stderr, "Erro: Nao foi possivel criar tarefa %d.\n", i + 1);
  			return -1;
  		}
  	}
  	
  	for (i=0; i<trab; i++)
  		if (pthread_join(tid[i], NULL)) {
      		printf("Erro: Nao foi possivel esperar por tarefa.\n");
      		return -1;
    	}
    
    dm2dPrint(matrix);
	
    if (barrier_destroy(&barrier)) {
  		fprintf(stderr, "Erro: Nao foi possivel destruir barriera.\n");
    	return -1;
  	}
	
	dm2dFree(matrix);
    dm2dFree(matrixAux);
	
    free(tid);
    free(args);
    free(threadCurrMaxD);
	
  	return 0;
}
