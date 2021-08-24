#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <wait.h>
#include <stdbool.h>

#include "matrix2d.h"
#include "barrier.h"

int iter, trab, columns, periodoS, child_pid = -1;
double maxD, currMaxD, *threadCurrMaxD;
char* fichS, *auxFichS;
DoubleMatrix2D *matrix, *matrixAux;
barrier_t barrier;
FILE *file;

struct {
	bool sigint, sigalrm;
} flags;

pthread_mutex_t flag_mutex;

void createSnapshot() {
	if ((file = fopen(auxFichS, "w")) == NULL) {
		fprintf(stderr, "Erro na criacao de ficheiro auxiliar.\n");
		exit(EXIT_FAILURE);
	}
				
	dm2dPrintToFile(matrix, file);
				
	if (fclose(file) == EOF) {
		fprintf(stderr, "Erro ao fechar ficheiro auxiliar.\n");
		exit(EXIT_FAILURE);
	}
				
	if (rename(auxFichS, fichS)) {
		fprintf(stderr, "Erro ao substituir ficheiro auxiliar.\n");
		exit(EXIT_FAILURE);
	}
				
	exit(EXIT_SUCCESS);
}

void endIter_simulMatrix(void *args) {
	DoubleMatrix2D *tmp;
	
	tmp = matrix;
    matrix = matrixAux;
    matrixAux = tmp;
    
    if (pthread_mutex_lock(&(flag_mutex))) {
    	fprintf(stderr, "Erro ao bloquear flag_mutex.\n");
    	exit(EXIT_FAILURE);
    }
    
    int status = 0, wpid;
    
    if (flags.sigint || flags.sigalrm) {
    	if (flags.sigint)
			iter = 1;
		else
			flags.sigalrm = 0;
    	
		if ((wpid = waitpid(child_pid, &status, WNOHANG)) > 0 || child_pid == -1) {
			if (!WIFEXITED(status))
				fprintf(stderr, "Erro na execucao do processo filho e backup nao foi efetizado.\n");
			
			if (!(child_pid = fork())) 
				createSnapshot();
			else if (child_pid == -1)
				fprintf(stderr, "Erro na criacao do processo filho e o backup nao sera feito.\n");
				
		} else if (wpid == 0)
			fprintf(stderr, "Processo filho anterior ainda nao terminou e o backup nao sera feito.\n");
			
		else
			fprintf(stderr, "Erro na espera por processo filho e o backup nao sera feito.\n");
	}
	
	if (pthread_mutex_unlock(&(flag_mutex))) {
    	fprintf(stderr, "Erro ao desbloquear flag_mutex.\n");
    	exit(EXIT_FAILURE);
    }
	
	double max = threadCurrMaxD[0];
	int i;
	
	for (i=1; i<trab; i++)
		if (threadCurrMaxD[i] > max)
			max = threadCurrMaxD[i];
	
	currMaxD = max;
	
	iter--;
}

typedef struct {
	int id, firstLine, lastLine;
} simulMatrix_args;

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

void sig_handler(int sig) {
	if (pthread_mutex_lock(&(flag_mutex))) {
    	fprintf(stderr, "Erro ao bloquear flag_mutex.\n");
    	exit(EXIT_FAILURE);
    }
    
    if (sig == SIGALRM) {
    	flags.sigalrm = 1;
    	alarm(periodoS);
    } else
		flags.sigint = 1;
	
	if (pthread_mutex_unlock(&(flag_mutex))) {
    	fprintf(stderr, "Erro ao desbloquear flag_mutex.\n");
    	exit(EXIT_FAILURE);
    }
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
  	if (argc != 11) {
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
  	fichS = argv[9];
  	periodoS = parse_integer_or_exit(argv[10], "periodoS");
	
  	fprintf(stderr, "Argumentos: N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d maxD=%.4f trab=%d fichS=%s periodoS=%d\n", N, tEsq, tSup, tDir, tInf, iter, maxD, trab, fichS, periodoS);
	
  	if (N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 || trab < 1 || N%trab != 0 || maxD < 0 || fichS == NULL || periodoS < 0)  {
    	fprintf(stderr, "Erro: Argumentos invalidos.\n");
    	return -1;
  	}
	
  	if ((file = fopen(fichS, "r")) != NULL) {
  		if ((matrix = readMatrix2dFromFile(file, N+2, N+2)) == NULL) {
			fprintf(stderr, "Erro: Nao foi possivel ler matriz do ficheiro.\n");
			return -1;
		}
		
		fclose(file);
  	} else {
	  	if ((matrix = dm2dNew(N+2, N+2)) == NULL) {
			fprintf(stderr, "Erro: Nao foi possivel alocar memoria para a matriz.\n");
			return -1;
	  	}
	
	  	dm2dSetLineTo(matrix, 0, tSup);
	  	dm2dSetLineTo(matrix, N+1, tInf);
	  	dm2dSetColumnTo(matrix, 0, tEsq);
	  	dm2dSetColumnTo(matrix, N+1, tDir);
	}
	
	if ((matrixAux = dm2dNew(N+2, N+2)) == NULL) {
		fprintf(stderr, "Erro: Nao foi possivel alocar memoria para a matriz auxiliar.\n");
		return -1;
	}
	
	dm2dCopy(matrixAux, matrix);
	
	if ((auxFichS = (char*)malloc(strlen(fichS)+2)) == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para o auxFichS.\n");
    	return -1;
    }
    
  	strcat(strcpy(auxFichS, "~"), fichS);
  	
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
  	
  	if ((threadCurrMaxD = (double*)malloc(trab*sizeof(double))) == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para as threadCurrMaxD.\n");
    	return -1;
  	}
	
  	columns = N + 2;
  	currMaxD = maxD + 1;
  	
  	sigset_t sigset;
  	
  	if (sigemptyset(&sigset)) {
  		fprintf(stderr, "Erro: Nao foi possivel criar o set de sinais vazio.\n");
    	return -1;
  	}
  	
  	if (sigaddset(&sigset, SIGINT) || sigaddset(&sigset, SIGALRM)) {
  		fprintf(stderr, "Erro: Nao foi possivel adicionar sinais ao set.\n");
    	return -1;
  	}
  	
  	if (pthread_sigmask(SIG_BLOCK, &sigset, NULL)) {
  		fprintf(stderr, "Erro: Nao foi possivel bloquear os sinais das threads.\n");
    	return -1;
  	}
  	
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
  	
  	if (pthread_mutex_init(&(flag_mutex), NULL)) {
  		fprintf(stderr, "Erro ao inicializar flag_mutex.\n");
		return -1;
	}
		
	struct sigaction sigact;
  	
  	sigact.sa_handler = sig_handler;
  	sigact.sa_flags = 0;
  	
  	if (sigemptyset(&(sigact.sa_mask))) {
  		fprintf(stderr, "Erro: Nao foi possivel criar o set de sinais vazio.\n");
    	return -1;
  	}
  	
  	if (sigaction(SIGINT, &sigact, NULL)) {
  		fprintf(stderr, "Erro: Nao foi possivel associar o sigaction.\n");
    	return -1;
  	}
  	
  	if (sigaction(SIGALRM, &sigact, NULL)) {
  		fprintf(stderr, "Erro: Nao foi possivel associar o sigaction.\n");
    	return -1;
  	}
  	
  	if (pthread_sigmask(SIG_UNBLOCK, &sigset, NULL)) {
  		fprintf(stderr, "Erro: Nao foi possivel desbloquear os sinais na main.\n");
    	return -1;
  	}
  	
  	alarm(periodoS);
  	
  	for (i=0; i<trab; i++)
  		if (pthread_join(tid[i], NULL)) {
      		printf("Erro: Nao foi possivel esperar por tarefa.\n");
      		return -1;
    	}
    
    if (pthread_sigmask(SIG_BLOCK, &sigset, NULL)) {
  		fprintf(stderr, "Erro: Nao foi possivel bloquear os sinais das threads.\n");
    	return -1;
  	}
  	
  	if (pthread_mutex_destroy(&(flag_mutex))) {
    	fprintf(stderr, "Erro ao destruir mutex.\n");
    	return -1;
  	}
    
    if (!flags.sigint) {
    	dm2dPrint(matrix);
    
    	if (unlink(fichS) && errno != ENOENT)
      		fprintf(stderr, "Erro: Nao foi possivel apagar ficheiro.\n");
    }
    
    int status = 0;
    
    if (wait(&status) == -1 && child_pid != -1)
    	fprintf(stderr, "Erro na espera por processo filho.\n");
    
    if (!WIFEXITED(status))
    	fprintf(stderr, "Erro na execucao do processo filho e backup nao foi efetizado.\n");
	
    if (barrier_destroy(&barrier))
  		fprintf(stderr, "Erro: Nao foi possivel destruir barriera.\n");
	
	dm2dFree(matrix);
    dm2dFree(matrixAux);
	
	free(auxFichS);
    free(tid);
    free(args);
    free(threadCurrMaxD);
	
  	return 0;
}
