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

#include "bsg_manycore.h"
#include "barrier.c"
#include "phasedetector.h"
//#include "mpi.c"
// these are private variables
// we do not make them volatile
// so that they may be cached

int bsg_x = -1;
int bsg_y = -1;


//int RR_index;
int x_index = 0;
int y_index = 0;
int sent[bsg_tiles_X*bsg_tiles_Y];
int received[bsg_tiles_X*bsg_tiles_Y];
//int count[bsg_tiles_X*bsg_tiles_Y];
//int inc = 0;
int data = 0;
int data2 = 0;

int bsg_set_tile_x_y()
{
  // everybody stores to tile 0,0
  bsg_remote_store(0,0,&bsg_x,0);
  bsg_remote_store(0,0,&bsg_y,0);

  for(int i = 0; i < bsg_tiles_X*bsg_tiles_Y; i++) {
    bsg_volatile_access(received[i]) = 1;
  }

  // make sure memory ops above are not moved down
  bsg_compiler_memory_barrier();

  // wait for my tile number to change
  bsg_wait_while((bsg_volatile_access(bsg_x) == -1) || (bsg_volatile_access(bsg_y) == -1));

  // make sure memory ops below are not moved above
  bsg_compiler_memory_barrier();

  // head of each column is responsible for
  // propagating to next column
  if ((bsg_x == 0)
      && ((bsg_y + 1) != bsg_tiles_Y)
    )
  {
    bsg_remote_store(0,bsg_y+1,&bsg_x,bsg_x);
    bsg_remote_store(0,bsg_y+1,&bsg_y,bsg_y+1);
  }

  // propagate across each rowez
  if ((bsg_x+1) != bsg_tiles_X)
  {
    bsg_remote_store(bsg_x+1,bsg_y,&bsg_x,bsg_x+1);
    bsg_remote_store(bsg_x+1,bsg_y,&bsg_y,bsg_y);
  }
}




//0 = -1, 1 = 1
static const coef_t c_I[N] = {	1,    	0,    	1,    	0,    	0,    	0,	1,    	1,    
				0,    	0,    	0,    	1,	1,  	0,    	1,    	0,    
				0,    	0,	0,    	1,    	1,    	1,    	1,    	1,
				0,    	0,    	1,    	1,    	1, 	0,    	0,    	0 
};

static const coef_t c_Q[N] = {	0,    	0,    	1,    	0,    	1,	0,    	1,    	0,    
				0,    	0,  	0,    	1,    	0,   	1,    	0,	1,    
				1,    	0,    	1,    	0,	0,    	1,    	0,    	1,    
				1,  	1,    	1,    	0,    	1,    	0,  	1,    	1 
};

static int regs_I[N];
static int regs_Q[N];

void shift_I (int x) {
	int i;
	for(i=N-1;i>0;i--){
		regs_I[i] = regs_I[i-1];
	}
	regs_I[0]=x;
}

void shift_Q (int x) {
	int i;
	for(i=N-1;i>0;i--){
		regs_Q[i] = regs_Q[i-1];
	}
	regs_Q[0]=x;
}

void accum_II(int *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_I[i] ? *acc+regs_I[i] : *acc-regs_I[i];

	}
}

void accum_IQ(int *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_Q[i] ? *acc+regs_I[i] : *acc-regs_I[i];
	}
}

void accum_QI(int *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_I[i] ? *acc+regs_Q[i] : *acc-regs_Q[i];
	}
}

void accum_QQ(int *acc) {
	int i;
	*acc=0;
	for(i=N-1;i>=0;i--){
		*acc = c_Q[i] ? *acc+regs_Q[i] : *acc-regs_Q[i];
	}
}

int main()
{
  int i=0,x=-1,lock_stat_ptr=0x8, lock_req_ptr=0xC, dummy;
  //volatile int * xp = (volatile int *) (0x80000001);
  //bsg_remote_int_ptr lock_req_ptr = bsg_remote_ptr(1,1,0xC);
  
  bsg_set_tile_x_y();

  bsg_remote_ptr_io_store(0,0x1260,bsg_x);
  bsg_remote_ptr_io_store(0,0x1264,bsg_y);

  bsg_remote_ptr_io_store(0,0x1234,0x13);


/*
 * Put fir code here
 */

  //(int I, int Q, int *X, int *Y) {
  int I, Q;
  int * X;
  int * Y;

  int II, IQ, QI, QQ;
	
  shI: shift_I(I);
  shQ: shift_Q(Q);

  aII: accum_II(&II);
  aQI: accum_QI(&QI);
  aIQ: accum_IQ(&IQ);
  aQQ: accum_QQ(&QQ);

  *X = II+QQ;
  *Y = QI-IQ;
}


