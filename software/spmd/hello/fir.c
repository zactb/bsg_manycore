/*
	Filename: fir.cpp
		Complex FIR or Match filter
		firI1 and firI2 share coef_t c[N]
		firQ1 and firQ2 share coef_t c[N]
		
	INPUT:
		I: signal for I sample
		I: signal for Q sample

	OUTPUT:
		X: filtered output
		Y: filtered output

*/

#include "phasedetector.h"
////#include "stdio.h"
#include "FPMult.h"

//0 = -1, 1 = 1

static data_t regs_I3[N*2];
static data_t regs_Q3[N*2];

//for 2 integer FP
/*
static const coef_t c_I3[2*N] = {0x1000000, 0, 0x0, 0, 0x1000000, 0, 0x0, 0,
                                 0x0,       0, 0x0, 0, 0x1000000, 0, 0x1000000, 0,
				 0x0, 0, 0x0, 0, 0x0, 0, 0x1000000, 0,
				 0x1000000, 0, 0x0, 0, 0x1000000, 0, 0x0, 0,
				 0x0, 0, 0x0, 0, 0x0, 0,  0x1000000, 0,
				 0x1000000, 0, 0x1000000, 0, 0x1000000, 0, 0x1000000, 0,
                                 0x0,       0, 0x0, 0, 0x1000000, 0, 0x1000000, 0,
				 0x1000000, 0, 0x0, 0, 0x0, 0, 0x0, 0};


static const coef_t c_Q3[2*N] = {0x0, 0, 0x0, 0, 0x1000000, 0, 0x0, 0,
				 0x1000000, 0, 0x0, 0, 0x1000000, 0, 0x0, 0,
				 0x0, 0, 0x0, 0, 0x0,  0, 0x1000000, 0,
				 0x0, 0, 0x1000000, 0, 0x0, 0,  0x1000000, 0,
				 0x1000000, 0, 0x1000000, 0, 0x1000000, 0, 0x0, 0,
				 0x0, 0, 0x1000000, 0, 0x0, 0, 0x1000000, 0,
				 0x1000000, 0, 0x1000000, 0, 0x1000000, 0, 0x0, 0,
				 0x1000000, 0, 0x0, 0, 0x1000000, 0, 0x1000000, 0};
				 */

//for my orignal FP
static const coef_t c_I2[N] = {0x10000, 0x0, 0x10000, 0x0, 
                                 0x0,       0x0, 0x10000, 0x10000, 
				 0x0, 0x0, 0x0,  0x10000, 
				 0x10000, 0x0, 0x10000, 0x0,
				 0x0, 0x0, 0x0,  0x10000, 
				 0x10000, 0x10000, 0x10000, 0x10000,
                                 0x0,       0x0, 0x10000, 0x10000, 
				 0x10000, 0x0, 0x0, 0x0};


static const coef_t c_Q2[N] = {0x0, 0x0, 0x10000, 0x0, 
				 0x10000, 0x0, 0x10000, 0x0,
				 0x0, 0x0, 0x0,  0x10000,
				 0x0, 0x10000, 0x0, 0x10000, 
				 0x10000, 0x0, 0x10000, 0x0,
				 0x0, 0x10000, 0x0, 0x10000,
				 0x10000, 0x10000, 0x10000, 0x0,
				 0x10000, 0x0, 0x10000, 0x10000};

//for floats
/*
static const coef_t c_I[N] = {1,    0,    1,    0,    
			      0,    0,    1,    1,    
			      0,    0,    0,    1,
			      1,    0,    1,    0,    
			      0,    0,    0,    1,
			      1,    1,    1,    1,
			      0,    0,    1,    1, 
			      1,    0,    0,    0};

static const coef_t c_Q[N] = {0,    0,    1,    0,   1,    0,    1,    0,   0,    0,	  0,    1, 0,    1,    0,    1, 1,    0,    1,    0,  0,    1,    0,    1,     1,    1,    1,    0, 1,    0, 1,    1};

*/
static data_t1 regs_I1[N];
static data_t1 regs_Q1[N];

static data_t regs_I[N];
static data_t regs_Q[N];

/*
void shift_I3 (data_t x[]) {
	int i;
	for(i=N-1;i>0;i--){
		regs_I3[2*i] = regs_I3[2*i-2];
		regs_I3[2*i+1] = regs_I3[2*i-1];
	}
	regs_I3[0]=x[0];
	regs_I3[1] = x[1];
}
*/

void shift_I (data_t x) {
	int i;
	for(i=N-1;i>0;i--){
		regs_I[i] = regs_I[i-1];
	}
	regs_I[0]=x;
}

/*
void shift_Q3 (data_t x[]) {
	int i;
	for(i=N-1;i>0;i--){
		regs_Q3[2*i] = regs_Q3[2*i-2];
		regs_Q3[2*i+1] = regs_Q3[2*i-1];
	}
	regs_Q3[0]=x[0];
	regs_Q3[1]=x[1];
}
*/

void shift_Q (data_t x) {
	int i;
	for(i=N-1;i>0;i--){
		regs_Q[i] = regs_Q[i-1];
	}
	regs_Q[0]=x;
}

/*
void accum_II3(acc_t *acc) {
	int i;
	acc[0] = 0;
	acc[1] = 0;
	for(i=N-1;i>=0;i--){
	  //printf("FP: *acc = %x %x\n", acc[0], acc[1]);
	  //printf("FP: c_I3[%d] = %d %d\n", i, c_I3[2*i], c_I3[2*i+1]);
	  if ( (c_I3[2*i]) || (c_I3[2*i+1])){
	    FPAdd( acc[0], acc[1], regs_I3[2*i], regs_I3[2*i+1], &acc[0], &acc[1]);
	  //printf("FPADD: *acc = %d %d\n", acc[0], acc[1]);
	  }
	  else{
	    FPSub( acc[0], acc[1], regs_I3[2*i], regs_I3[2*i+1], &acc[0], &acc[1]);
	  //printf("FPSUB: *acc = %d %d\n", acc[0], acc[1]);
          }
	  //printf("FP: regsI[%d] = %d %d\n", i, regs_I3[2*i], regs_I3[2*i+1]);
	  //printf("FP: *acc = %d %d\n", acc[0], acc[1]);
	}
}
*/

void accum_II(acc_t *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
	  //printf("D: *acc = %d\n", *acc);
	  //printf("D: , c_I2[%d] = %d\n", i, c_I2[i]);
		*acc = c_I2[i] ? *acc+regs_I[i] : *acc-regs_I[i];
	  //printf("D: , regs_I[%d] = %d\n", i, regs_I[i]);
	  //printf("D: *acc = %d\n", *acc);

	}
}

/*
void accum_IQ3(acc_t *acc) {
	int i;
	acc[0] = 0;
	acc[1] = 0;
	for(i=N-1;i>=0;i--){
	  if ( (c_Q3[2*i]) || (c_Q3[2*i+1])){
	    FPAdd( acc[0], acc[1], regs_I3[2*i], regs_I3[2*i+1], &acc[0], &acc[1]);
	  }
	  else{
	    FPSub( acc[0], acc[1], regs_I3[2*i], regs_I3[2*i+1], &acc[0], &acc[1]);
          }
	}
}
*/

void accum_IQ(acc_t *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_Q2[i] ? *acc+regs_I[i] : *acc-regs_I[i];
	}
}


/*
void accum_QI3(acc_t *acc) {
	int i;
	acc[0] = 0;
	acc[1] = 0;
	for(i=N-1;i>=0;i--){
	  //printf("FP: *acc = %d %d\n", acc[0], acc[1]);
	  //printf("FP: c_I3[%d] = %d %d\n", i, c_I3[2*i], c_I3[2*i+1]);
	  //printf("FP: , regs_Q3[%d] = %d %d\n", i, regs_Q3[2*i], regs_Q3[2*i+1]);
	  if ( (c_I3[2*i]) || (c_I3[2*i+1])){
	    FPAdd( acc[0], acc[1], regs_Q3[2*i], regs_Q3[2*i+1], &acc[0], &acc[1]);
	  }
	  else{
	    FPSub( acc[0], acc[1], regs_Q3[2*i], regs_Q3[2*i+1], &acc[0], &acc[1]);
          }
	  //printf("FP: *acc = %d %d\n", acc[0], acc[1]);
	  //printf("D: *acc = %d\n", *acc);
        }
}
*/

void accum_QI(acc_t *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
	  //printf("D: *acc = %d\n", *acc);
	  //printf("D: , c_I2[%d] = %d\n", i, c_I2[i]);
	  //printf("D: , regs_Q[%d] = %d\n", i, regs_Q[i]);
		*acc = c_I2[i] ? *acc+regs_Q[i] : *acc-regs_Q[i];
	  //printf("D: *acc = %d\n", *acc);
	}
}

/*
void accum_QQ3(acc_t *acc) {
	int i;
	acc[0] = 0;
	acc[1] = 0;
	for(i=N-1;i>=0;i--){
	  if ( ( c_Q3[2*i]) || (c_Q3[2*i+1])){
	    FPAdd( acc[0], acc[1], regs_Q3[2*i], regs_Q3[2*i+1], &acc[0], &acc[1]);
	  }
	  else{
	    FPSub( acc[0], acc[1], regs_Q3[2*i], regs_Q3[2*i+1], &acc[0], &acc[1]);
          }
        }
}

void fir3 (data_t I[], data_t Q[], data_t *X, acc_t *Y) {
	acc_t Y_last=*Y, X_last=*X;
	acc_t II[2], IQ[2], QI[2], QQ[2];
	
	shI: shift_I3(I);
	shQ: shift_Q3(Q);

	aII: accum_II3(II);
	aQI: accum_QI3(QI);
	aIQ: accum_IQ3(IQ);
	aQQ: accum_QQ3(QQ);
	//printf("FP: QI = %d %d\n", QI[0], QI[1]);
	//printf("FP: IQ = %d %d\n", IQ[0], IQ[1]);
	//printf("FP: QQ = %d %d\n", QQ[0], QQ[1]);

	FPAdd( II[0], II[1], QQ[0], QQ[1], &X[0], &X[1] );
	FPSub( QI[0], QI[1], IQ[0], IQ[1], &Y[0], &Y[1] );

	// *X = II+QQ;
	// *Y = QI-IQ;
	//printf ("X = %f\tY = %f\n", X, Y);
}
*/



void accum_QQ(acc_t *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_Q2[i] ? *acc+regs_Q[i] : *acc-regs_Q[i];
	}
}

void fir_0 (data_t I, data_t Q, data_t *X, acc_t *Y, int a) {
	acc_t Y_last=*Y, X_last=*X;
	acc_t II, IQ, QI, QQ;
	if (a) {
	  shI: shift_I(I);
	  shQ: shift_Q(Q);
	}
	aII: accum_II(&II);
}
void fir_1 (data_t I, data_t Q, data_t *X, acc_t *Y, int a) {
	acc_t Y_last=*Y, X_last=*X;
	acc_t II, IQ, QI, QQ;
	if (a) {
	  shI: shift_I(I);
	  shQ: shift_Q(Q);
	}
	aQI: accum_QI(&QI);
}
void fir_2 (data_t I, data_t Q, data_t *X, acc_t *Y, int a) {
	acc_t Y_last=*Y, X_last=*X;
	acc_t II, IQ, QI, QQ;
	if (a) {
	  shI: shift_I(I);
	  shQ: shift_Q(Q);
	}
	aIQ: accum_QI(&IQ);
}
void fir_3 (data_t I, data_t Q, data_t *X, acc_t *Y, int a) {
	acc_t Y_last=*Y, X_last=*X;
	acc_t II, IQ, QI, QQ;
	if (a) {
	  shI: shift_I(I);
	  shQ: shift_Q(Q);
	}
	aQQ: accum_QI(&QQ);
}

void fir (data_t I, data_t Q, data_t *X, acc_t *Y, int a) {
	acc_t Y_last=*Y, X_last=*X;
	acc_t II, IQ, QI, QQ;
	
	if (a == 1){
	  shI: shift_I(I);
	  shQ: shift_Q(Q);
	  aII: accum_II(&II);
	  *X = II;
	}
        else if (a == 2){
	  aQI: accum_QI(&QI);
	  *X = QI;
	}
	else if (a == 3){
	  aIQ: accum_IQ(&IQ);
	  *X = IQ;
	}
	else if (a == 4){
	  aQQ: accum_QQ(&QQ);
	  *X = QQ;
	}
	/*
	printf("D: II = %d\n", II);
	printf("D: QI = %d\n", QI);
	printf("D: IQ = %d\n", IQ);
	printf("D: QQ = %d\n", QQ);
	*/

	*X = II+QQ;
	*Y = QI-IQ;
	//printf ("X = %f\tY = %f\n", X, Y);
}



/*
void shift_I1 (data_t1 x) {
	int i;
	for(i=N-1;i>0;i--){
		regs_I1[i] = regs_I1[i-1];
	}
	regs_I1[0]=x;
}

void shift_Q1 (data_t1 x) {
	int i;
	for(i=N-1;i>0;i--){
		regs_Q1[i] = regs_Q1[i-1];
	}
	regs_Q1[0]=x;
}

void accum_II1(acc_t1 *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_I[i] ? *acc+regs_I1[i] : *acc-regs_I1[i];

	}
}

void accum_IQ1(acc_t1 *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_Q[i] ? *acc+regs_I1[i] : *acc-regs_I1[i];
	}
}

void accum_QI1(acc_t1 *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_I[i] ? *acc+regs_Q1[i] : *acc-regs_Q1[i];
	}
}

void accum_QQ1(acc_t1 *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_Q[i] ? *acc+regs_Q1[i] : *acc-regs_Q1[i];
	}
}

void fir2 (data_t1 I, data_t1 Q, data_t1 *X, acc_t1 *Y) {
	acc_t1 Y_last=*Y, X_last=*X;
	acc_t1 II, IQ, QI, QQ;
	
	shI: shift_I1(I);
	shQ: shift_Q1(Q);

	aII: accum_II1(&II);
	aQI: accum_QI1(&QI);
	aIQ: accum_IQ1(&IQ);
	aQQ: accum_QQ1(&QQ);

	*X = II+QQ;
	*Y = QI-IQ;
	//printf ("X = %f\tY = %f\n", X, Y);
}
*/


