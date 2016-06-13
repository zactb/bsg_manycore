#include "input_i.h"
#include "input_q.h"
#include "out.gold.h"
#include "bsg_manycore.h"
#include <stdlib.h>
#include "phasedetector.h"
#include <stdio.h>
#include <math.h>
#include "barrier.c"
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



int main()
{
  int i=0,x=-1,lock_stat_ptr=0x8, lock_req_ptr=0xC, dummy;
//  volatile int * xp = (volatile int *) (0x80000001);
  //bsg_remote_int_ptr lock_req_ptr = bsg_remote_ptr(1,1,0xC);
  
  bsg_set_tile_x_y();

  bsg_remote_ptr_io_store(0,0x1260,bsg_x);
  bsg_remote_ptr_io_store(0,0x1264,bsg_y);

  bsg_remote_ptr_io_store(0,0x1234,0x13);



  const int    SAMPLES=64;

  int s_I = 0, s_Q=0;
  int signal_I, signal_Q, output_R, output_Theta;
  signal_I = 0;
  signal_Q = 0;

  int o_R=0, o_T = 0;

  int gold_R;
  int gold_Theta;

  int error_R=0;
  int error_Theta=0;

  int sum_R=0;
  int sum_R_gold=0;
  int sum_Theta=0;
  int sum_Theta_gold=0;


  for (i=0;i<SAMPLES;i++) {
    gold_R = gold[i*2];
    gold_Theta = gold[i*2+1];
    signal_I = input_i[i];
    signal_Q = input_q[i];

    int A = 0;
    int B = 0;
    fir(signal_I, signal_Q, &A, &B);
    cordiccart2pol(A, B, &o_R, &o_T);

    error_R = error_R + abs(o_R - gold_R);
    error_Theta = error_Theta + abs(o_T - gold_Theta);
    sum_R_gold = sum_R_gold+ (abs(gold_R) >> 8);
    sum_R = sum_R+ (abs(o_R) >> 8);
    sum_Theta_gold = sum_Theta_gold+ abs(gold_Theta);
    sum_Theta = sum_Theta + abs(o_T);
  }

  //Write the results somewhere?
/*
  //Checking error
  if(error_R<1677 and error_Theta<1677) {
	  printf("error_R=%f, error_Theta=%f\n", float(error_R)/16777216, float(error_Theta)/16777216);
	  printf("percent error Theta=%f\n", float(100*abs((sum_Theta - sum_Theta_gold)))/sum_Theta_gold);
	  printf("percent error R=%f\n", float(100*abs((sum_R - sum_R_gold)))/sum_R_gold);
	  printf("PASS\n");
  }
  else{
	  printf("error_R=%f, error_Theta=%f\n", float(error_R)/16777216, float(error_Theta)/16777216);
	  printf("percent error Theta=%f\n", fabs(float(100*(sum_Theta - sum_Theta_gold))/sum_Theta_gold));
	  printf("percent error R=%f\n", fabs(float(100*(sum_R - sum_R_gold))/sum_R_gold));
	  printf("FAIL\n");
  }
*/


  bsg_wait_while(1);
}
