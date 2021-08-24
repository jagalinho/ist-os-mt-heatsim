#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>

#include "matrix2d.h"
#include "mplib4.h"

void* simulMatrix(void *arg) {
	int myid = *(int*)arg, i, j;
	
	int trab, iter, lines, columns;
	double maxD, diff;
	
	if ((receberMensagem(0, myid, &trab, sizeof(int)) != sizeof(int))
	|| (receberMensagem(0, myid, &iter, sizeof(int)) != sizeof(int))
	|| (receberMensagem(0, myid, &lines, sizeof(int)) != sizeof(int))
	|| (receberMensagem(0, myid, &columns, sizeof(int)) != sizeof(int))
	|| (receberMensagem(0, myid, &maxD, sizeof(double)) != sizeof(double))) {
		fprintf(stderr, "Erro: Nao foi possivel receber mensagem.\n");
		exit(EXIT_FAILURE);
	}
	
	DoubleMatrix2D *matrix = dm2dNew(lines, columns), *matrixAux = dm2dNew(lines, columns), *tmp;
	
	if (matrix == NULL || matrixAux == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para as matrizes.\n");
    	exit(EXIT_FAILURE);
  	}
	
	size_t size_buffer = lines*columns*sizeof(double);
	double *buffer = (double*)malloc(size_buffer);
	
	if (buffer == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para o buffer da thread.\n");
    	exit(EXIT_FAILURE);
  	}
	
	if (receberMensagem(0, myid, buffer, size_buffer) != size_buffer) {
		fprintf(stderr, "Erro: Nao foi possivel receber mensagem.\n");
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<lines; i++) {
		dm2dSetLine(matrix, i, &buffer[i*columns]);
	}
	
	dm2dCopy(matrixAux, matrix);
	
	double currMaxD = maxD + 1;
	
	while (iter && (currMaxD > maxD)) {
		currMaxD = 0;
		
		for (i=1; i<lines-1; i++)
      		for (j=1; j<columns-1; j++) {
        		dm2dSetEntry(matrixAux, i, j, (dm2dGetEntry(matrix, i-1, j) + dm2dGetEntry(matrix, i+1, j) + dm2dGetEntry(matrix, i, j-1) + dm2dGetEntry(matrix, i, j+1)) / 4.0);
        		diff = fabs(dm2dGetEntry(matrixAux, i, j) - dm2dGetEntry(matrix, i, j));
        		if (diff > currMaxD)
        			currMaxD = diff;
        	}
		
		tmp = matrixAux;
    	matrixAux = matrix;
    	matrix = tmp;
    	
		if (myid % 2) {
			if (myid != 1) {
				if (receberMensagem(myid-1, myid, dm2dGetLine(matrix, 0), columns*sizeof(double)) != columns*sizeof(double)) {
					fprintf(stderr, "Erro: Nao foi possivel receber mensagem.\n");
					exit(EXIT_FAILURE);
				}
				if (enviarMensagem(myid, myid-1, dm2dGetLine(matrix, 1), columns*sizeof(double)) != columns*sizeof(double)) {
					fprintf(stderr, "Erro: Nao foi possivel enviar mensagem.\n");
					exit(EXIT_FAILURE);
				}
			}
			if (myid != trab) {
				if (receberMensagem(myid+1, myid, dm2dGetLine(matrix, lines-1), columns*sizeof(double)) != columns*sizeof(double)) {
					fprintf(stderr, "Erro: Nao foi possivel receber mensagem.\n");
					exit(EXIT_FAILURE);
				}
				if (enviarMensagem(myid, myid+1, dm2dGetLine(matrix, lines-2), columns*sizeof(double)) != columns*sizeof(double)) {
					fprintf(stderr, "Erro: Nao foi possivel enviar mensagem.\n");
					exit(EXIT_FAILURE);
				}
			}
		} else {
			if (enviarMensagem(myid, myid-1, dm2dGetLine(matrix, 1), columns*sizeof(double)) != columns*sizeof(double)) {
				fprintf(stderr, "Erro: Nao foi possivel enviar mensagem.\n");
				exit(EXIT_FAILURE);
			}
			if (receberMensagem(myid-1, myid, dm2dGetLine(matrix, 0), columns*sizeof(double)) != columns*sizeof(double)) {
				fprintf(stderr, "Erro: Nao foi possivel receber mensagem.\n");
				exit(EXIT_FAILURE);
			}
			if (myid != trab) {
				if (enviarMensagem(myid, myid+1, dm2dGetLine(matrix, lines-2), columns*sizeof(double)) != columns*sizeof(double)) {
					fprintf(stderr, "Erro: Nao foi possivel enviar mensagem.\n");
					exit(EXIT_FAILURE);
				}
				if (receberMensagem(myid+1, myid, dm2dGetLine(matrix, lines-1), columns*sizeof(double)) != columns*sizeof(double)) {
					fprintf(stderr, "Erro: Nao foi possivel receber mensagem.\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		
		if (enviarMensagem(myid, 0, &currMaxD, sizeof(double)) != sizeof(double)) {
			fprintf(stderr, "Erro: Nao foi possivel enviar mensagem.\n");
			exit(EXIT_FAILURE);
		}
		if (receberMensagem(0, myid, &currMaxD, sizeof(double)) != sizeof(double)) {
			fprintf(stderr, "Erro: Nao foi possivel receber mensagem.\n");
			exit(EXIT_FAILURE);
		}
    	
		iter--;
	}
	
	for (i=1; i<lines-1; i++)
		memcpy(&buffer[(i-1)*columns], dm2dGetLine(matrix, i), size_buffer/lines);

	if (enviarMensagem(myid, 0, buffer, (lines-2)*(size_buffer/lines)) != (lines-2)*(size_buffer/lines)) {
		fprintf(stderr, "Erro: Nao foi possivel enviar mensagem.\n");
		exit(EXIT_FAILURE);
	}
	
	free(buffer);
	
	dm2dFree(matrixAux);
	dm2dFree(matrix);
	
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
  	if (argc != 10) {
    	fprintf(stderr, "Erro: Numero invalido de argumentos.\n");
    	return -1;
  	}
	
  	int N = parse_integer_or_exit(argv[1], "N");
  	double tEsq = parse_double_or_exit(argv[2], "tEsq");
  	double tSup = parse_double_or_exit(argv[3], "tSup");
  	double tDir = parse_double_or_exit(argv[4], "tDir");
  	double tInf = parse_double_or_exit(argv[5], "tInf");
  	int iter = parse_integer_or_exit(argv[6], "iter");
  	double maxD = parse_double_or_exit(argv[7], "tInf");
  	int trab = parse_integer_or_exit(argv[8], "trab");
  	int csz = parse_integer_or_exit(argv[9], "csz");
	
  	fprintf(stderr, "Argumentos: N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d maxD=%.4f trab=%d csz=%d\n", N, tEsq, tSup, tDir, tInf, iter, maxD, trab, csz);
	
  	if (N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 || trab < 1 || N%trab != 0 || csz < 0) {
    	fprintf(stderr, "Erro: Argumentos invalidos.\n");
    	return -1;
  	}
	
  	DoubleMatrix2D *matrix = dm2dNew(N+2, N+2);
	
  	if (matrix == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para a matriz.\n");
    	return -1;
  	}
	
  	dm2dSetLineTo(matrix, 0, tSup);
  	dm2dSetLineTo(matrix, N+1, tInf);
  	dm2dSetColumnTo(matrix, 0, tEsq);
  	dm2dSetColumnTo(matrix, N+1, tDir);
  	
  	pthread_t *tid = (pthread_t*)malloc(trab*sizeof(pthread_t));
  	
  	if (tid == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para as thread id.\n");
    	return -1;
  	}
  	
  	if (inicializarMPlib(csz, trab+1) == -1) {
  		fprintf(stderr, "Erro: Nao foi possivel inicializar MPLib.\n");
  		return -1;
  	}
  	
  	int *id = (int*)malloc(trab*sizeof(int));
  	
  	if (id == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para o id.\n");
    	return -1;
  	}
  	
  	int lines = (N/trab)+2, columns = N+2;
  	
  	size_t size_buffer = lines*columns*sizeof(double);
  	double *buffer = (double*)malloc(size_buffer);
  	
  	if (buffer == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para o buffer.\n");
    	return -1;
  	}
  	
  	int i, j, k;
  	
  	for (i=0; i<trab; i++) {
  		id[i] = i+1;
  		
  		if (pthread_create(&tid[i], NULL, simulMatrix, &id[i])) {
  			fprintf(stderr, "Erro: Nao foi possivel criar tarefa %d.\n", id[i]);
  			return -1;
  		}

  		if ((enviarMensagem(0, id[i], &trab, sizeof(int)) != sizeof(int))
  		|| (enviarMensagem(0, id[i], &iter, sizeof(int)) != sizeof(int))
  		|| (enviarMensagem(0, id[i], &lines, sizeof(int)) != sizeof(int))
  		|| (enviarMensagem(0, id[i], &columns, sizeof(int)) != sizeof(int))
  		|| (enviarMensagem(0, id[i], &maxD, sizeof(double)) != sizeof(double))) {
  			fprintf(stderr, "Erro: Nao foi possivel enviar mensagem.\n");			
  			return -1;
  		}
  		
  		k = 0;
  		for (j=((lines-2)*i); j<=(lines-2)*(i+1)+1; j++) //ciclo redundante
  			memcpy(&buffer[k++*columns], dm2dGetLine(matrix, j), size_buffer/lines);
  		
  		if (enviarMensagem(0, id[i], buffer, size_buffer) != size_buffer) {
  			fprintf(stderr, "Erro: Nao foi possivel enviar mensagem.\n");
  			return -1;
  		}
  	}
  	
  	double currMaxD = maxD + 1, currMaxDThread;
  	
  	while (iter && (currMaxD > maxD)) {
  		currMaxD = 0;
  		
  		for (i=0; i<trab; i++) {
  			if (receberMensagem(i+1, 0, &currMaxDThread, sizeof(double)) != sizeof(double)) {
  				fprintf(stderr, "Erro: Nao foi possivel receber mensagem.\n");
  				return -1;
  			}
  			
  			if (currMaxDThread > currMaxD)
  				currMaxD = currMaxDThread;
  		}
  		
  		for (i=0; i<trab; i++)
  			if (enviarMensagem(0, i+1, &currMaxD, sizeof(double)) != sizeof(double)) {
  				fprintf(stderr, "Erro: Nao foi possivel enviar mensagem.\n");
  				return -1;
  			}
  		
  		iter--;
  	}
  	
  	for (i=0; i<trab; i++) {
  		if (receberMensagem(i+1, 0, buffer, (lines-2)*(size_buffer/lines)) != (lines-2)*(size_buffer/lines)) {
  			fprintf(stderr, "Erro: Nao foi possivel receber mensagem.\n");
  			return -1;
  		}
  		
  		k = 0;
  		for (j=((lines-2)*i)+1; j<=(lines-2)*(i+1); j++)
  			dm2dSetLine(matrix, j, &buffer[k++*columns]);
  	}
  	
  	dm2dPrint(matrix);
  	
  	for (i=0; i<trab; i++)
  		if (pthread_join(tid[i], NULL)) {
      		printf("Erro: Nao foi possivel esperar por tarefa.\n");
      		return -1;
    	}
    
    free(buffer);
    
    libertarMPlib();
    
    free(tid);
    free(id);
    
    dm2dFree(matrix);
  	
  	return 0;
}
