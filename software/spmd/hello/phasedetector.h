/*
	Filename: fir.h
*/
#ifndef PHASE_DETECTOR_H_
#define PHASE_DETECTOR_H_

//#include "ap_int.h"
//#include "ap_fixed.h"

#define N 32
#define NO_ITER 24
#define MAX_ITER 24
#ifdef BIT_ACCURATE
/*
typedef ap_int<1>	coef_t;
typedef ap_fixed<32,8>	data_t;
typedef ap_fixed<32,8>	acc_t;
typedef ap_fixed<18,1>	data_k; 
*/
#else
typedef int	coef_t;
typedef float	data_t;
typedef float	acc_t;
#endif

void phasedetector (
  int I,
  int Q,

  int *R,
  int *theta
);

void cordiccart2pol(int x, int y, int * r, int * theta);

void fir (
  int I,
  int Q,

  int *X,
  int *Y
  );


#endif
