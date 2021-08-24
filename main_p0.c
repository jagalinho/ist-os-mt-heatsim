#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "matrix2d.h"

DoubleMatrix2D *simul(DoubleMatrix2D *matrix, DoubleMatrix2D *matrixAux, int linhas, int colunas, int iter, double maxD) {

	DoubleMatrix2D *m, *aux, *tmp;
  	int i, j;
  	double value, diff, currMaxDiff = maxD + 1;

  	if (linhas < 2 || colunas < 2)
    	return NULL;

  	m = matrix;
  	aux = matrixAux;

  	while (iter && (currMaxDiff > maxD)) {
  		currMaxDiff = 0;
  		
    	for (i = 1; i < linhas - 1; i++)
      		for (j = 1; j < colunas - 1; j++) {
        		value = ( dm2dGetEntry(m, i-1, j) + dm2dGetEntry(m, i+1, j) + dm2dGetEntry(m, i, j-1) + dm2dGetEntry(m, i, j+1) ) / 4.0;
        		dm2dSetEntry(aux, i, j, value);
        		diff = fabs(value - dm2dGetEntry(m, i, j));
        		if (diff > currMaxDiff)
        			currMaxDiff = diff;
      		}
	
    	tmp = aux;
    	aux = m;
    	m = tmp;
    
    	iter--;
  	}

  	return m;
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

int main (int argc, char** argv) {
  	if (argc != 8) {
    	fprintf(stderr, "Erro: Numero invalido de argumentos.\n");
    	return -1;
  	}

  	int N = parse_integer_or_exit(argv[1], "N");
  	double tEsq = parse_double_or_exit(argv[2], "tEsq");
  	double tSup = parse_double_or_exit(argv[3], "tSup");
  	double tDir = parse_double_or_exit(argv[4], "tDir");
  	double tInf = parse_double_or_exit(argv[5], "tInf");
  	int iter = parse_integer_or_exit(argv[6], "iter");
  	double maxD = parse_double_or_exit(argv[7], "maxD");

  	DoubleMatrix2D *matrix, *matrixAux, *result;

  	fprintf(stderr, "Argumentos: N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d maxD=%.4f\n", N, tEsq, tSup, tDir, tInf, iter, maxD);

  	if (N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1) {
    	fprintf(stderr, "Erro: Argumentos invalidos.\n");
    	return -1;
  	}

  	matrix = dm2dNew(N+2, N+2);
  	matrixAux = dm2dNew(N+2, N+2);

  	if (matrix == NULL || matrixAux == NULL) {
    	fprintf(stderr, "Erro: Nao foi possivel alocar memoria para as matrizes.\n");
    	return -1;
  	}

  	int i;

  	for (i=0; i<N+2; i++)
    	dm2dSetLineTo(matrix, i, 0);

  	dm2dSetLineTo(matrix, 0, tSup);
  	dm2dSetLineTo(matrix, N+1, tInf);
  	dm2dSetColumnTo(matrix, 0, tEsq);
  	dm2dSetColumnTo(matrix, N+1, tDir);

  	dm2dCopy(matrixAux, matrix);

  	result = simul(matrix, matrixAux, N+2, N+2, iter, maxD);
  	
  	if (result == NULL) {
    	printf("Erro na simulacao.\n");
    	return -1;
  	}

  	dm2dPrint(result);

  	dm2dFree(matrix);
  	dm2dFree(matrixAux);

  	return 0;
}
