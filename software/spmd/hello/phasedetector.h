/*
	Filename: fir.h
*/
#ifndef PHASE_DETECTOR_H_
#define PHASE_DETECTOR_H_

/*
#include "ap_int.h"
#include "ap_fixed.h"
*/
#include "FPMult.h"

//const int N=32;

#define N 32
#define NO_ITER 24
#define MAX_ITER 24
/*
#ifdef BIT_ACCURATE
typedef ap_int<1>	coef_t;
typedef ap_fixed<32,8>	data_t;
typedef ap_fixed<32,8>	acc_t;
typedef ap_fixed<18,1>	data_k; 
#else
*/
typedef int	coef_t;
typedef int	data_t;
typedef int	acc_t;
typedef float	data_t1;
typedef float	acc_t1;
//#endif

void phasedetector3 (
  data_t *I,
  data_t *Q,

  data_t *R,
  data_t *theta
  );

void phasedetector2 (
  data_t1 I,
  data_t1 Q,

  data_t1 *R,
  data_t1 *theta
  );


void phasedetector (
  data_t I,
  data_t Q,

  data_t *R,
  data_t *theta
  );

void cordiccart2pol(data_t x, data_t y, data_t * r,  data_t * theta);
void cordiccart2pol3(data_t x[], data_t y[], data_t r[],  data_t theta[]);
void cordiccart2pol2(data_t1 x, data_t1 y, data_t1 * r,  data_t1 * theta);

void fir3 (
  data_t I[],
  data_t Q[],

  data_t *X,
  data_t *Y
  );

void fir_0 (data_t I, data_t Q, data_t *X, acc_t *Y, int a);
void fir_1 (data_t I, data_t Q, data_t *X, acc_t *Y, int a);
void fir_2 (data_t I, data_t Q, data_t *X, acc_t *Y, int a);
void fir_3 (data_t I, data_t Q, data_t *X, acc_t *Y, int a);

void fir (
  data_t I,
  data_t Q,

  data_t *X,
  data_t *Y,
  int a
  );

void fir2 (
  data_t1 I,
  data_t1 Q,

  data_t1 *X,
  data_t1 *Y
  );

#endif
