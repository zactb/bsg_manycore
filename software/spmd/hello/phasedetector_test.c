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

  int input[bsg_tiles_X*bsg_tiles_Y*2];

  int tileNum = bsg_volatile_access(bsg_x) + bsg_volatile_access(bsg_y)*bsg_tiles_X;
  for(int index = 0; index < bsg_tiles_X*bsg_tiles_Y; index++) {
    bsg_remote_store(index%bsg_tiles_X, index/bsg_tiles_X, (int *)(tileNum << 2), input);
  }
  
  barrier3(bsg_x, bsg_y, barr);

  const int    SAMPLES=64;

  int ptrs[bsg_tiles_X*bsg_tiles_Y];

  int s_I = 0, s_Q=0;
  int signal_I, signal_Q, output_R, output_Theta;
  signal_I = 0;
  signal_Q = 0;
  int A = 0;
  int B = 0;
  int o_R=0, o_T = 0;

  int received[bsg_tiles_X*bsg_tiles_Y];

  int gold_R;
  int gold_Theta;

  int error_R=0;
  int error_Theta=0;

  int sum_R=0;
  int sum_R_gold=0;
  int sum_Theta=0;
  int sum_Theta_gold=0;
  int core_ID = bsg_volatile_access(bsg_x) + bsg_tiles_X*bsg_volatile_access(bsg_y);
  int IIgranted = 0, IQgranted = 0, QIgranted = 0, QQgranted = 0;
  int II_lck, IQ_lck, QI_lck, QQ_lck;
  int IIindex, IQindex, QIindex, QQindex;


  //Iterate through sets of 32 data
  for(int k = 0; k < SAMPLES-32; k++) {
    //Iterate over cores
    for(int j = 0; 1; j = (j+1)%bsg_tiles_X*bsg_tiles_Y) {

      if(j == 2) {
        //Try requesting a lock
      //  bsg_remote_load(j%bsg_tiles_X, j/bsg_tiles_X, lock_req_ptr,x);
       // x=0xFF;
     while(x>3)   //bsg_remote_load(j/bsg_tiles_X, j%bsg_tiles_X, lock_stat_ptr, x); 
        bsg_remote_load(bsg_y, bsg_x, lock_stat_ptr, x); 
//bsg_print_time();
      if(x <= 3) { // || j==otherstuff
        //Find out which core that you want has its lock open
        if(!IIgranted) {
//bsg_print_time();
          //Calculating II
          for(int p = 1; p < 33; p++) 
            bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2))+p, input_i[p+k]);
          bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2))+33, core_ID);
          bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2)), 1);
          IIgranted = 1;
          IIindex = j;
          II_lck = x;

        } else if(!IQgranted) {
          //Calculating IQ
          for(int p = 1; p < 33; p++)
            bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2))+p, input_i[p+k]);
          bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2))+33, core_ID);
          bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, (int *)*(int *)(k<<2), 2);
          IQgranted = 1;
          IQindex = j;
          IQ_lck = x;
        } else if(!QIgranted) {
          //Calculating QI
          for(int p = 1; p < 33; p++)
            bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2))+p, input_q[p+k]);
          bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2))+33, core_ID);
          bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2)), 1);
          QIgranted = 1;
          QIindex = j;
 	  QI_lck = x;
  
        } else if(!QQgranted) {
          //Calculating QQ
          for(int p = 1; p < 33; p++)
            bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2))+p, input_q[p+k]);
          bsg_remote_store(j/bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2))+33, core_ID);
          bsg_remote_store(j%bsg_tiles_X, j/bsg_tiles_X, ((int *)*(int *)(k<<2)), 2);
          QQgranted = 1; 
          QQindex = j;
          QQ_lck = x;
        }}
      } 
      //Free the locks that are done
      if(received[IIindex] && II_lck != -1) { 
        bsg_remote_load(IIindex/bsg_tiles_X, IIindex%bsg_tiles_X, II_lck << 4, x);
        II_lck = -1;
      }
      if(received[IQindex] && IQ_lck != -1) {
        bsg_remote_load(IQindex/bsg_tiles_X, IQindex%bsg_tiles_X, IQ_lck << 4, x);
        IQ_lck = -1;
      }
      if(received[QIindex] && QI_lck != -1) {
        bsg_remote_load(QIindex/bsg_tiles_X, QIindex%bsg_tiles_X, QI_lck << 4, x);
        QI_lck = -1;
      }
      if(received[QQindex] && QQ_lck != -1) {
        bsg_remote_load(QQindex/bsg_tiles_X, QQindex%bsg_tiles_X, QQ_lck << 4, x);
        QQ_lck = -1;
      }
      //If they have all arrived
      if(received[IIindex] & received[IQindex] & received[QIindex] & received[QQindex]) {
        received[IIindex] = 0;
        received[IQindex] = 0;
        received[QIindex] = 0;
        received[QQindex] = 0;
        int A = input[IIindex]+input[QQindex];
        int B = input[QIindex]-input[IQindex];
  	//This was hardcoded
        bsg_remote_load(1, 1, lock_req_ptr,x);
        x=0xFF;
        //Busywait until core is obtained
        while(x > 3) {
          bsg_remote_load(bsg_x, bsg_y, lock_stat_ptr, x); 
        }
        int * cordic_ptr = &(((int *)*(int *)0x03)[x*3]);
        bsg_remote_store(1, 1, cordic_ptr, A);
        bsg_remote_store(1, 1, cordic_ptr+1, B);
        bsg_remote_store(1, 1, cordic_ptr+2, core_ID);
        bsg_wait_while(!received[x]);  
        //Free the lock
        bsg_remote_load(1, 1, x << 4, x);
        error_R = error_R + abs(input[2*3] - gold_R); 	//o_R
        error_Theta = error_Theta + abs(input[2*3+1] - gold_Theta);  //o_T
        IIgranted = 0;
        IQgranted = 0;
        QIgranted = 0;
        QQgranted = 0;
        break;
      }
    }
  }
    //Check incoming data bsg_wait_while(incoming data);
    //process incoming data
        
/* 
    sum_R_gold = sum_R_gold+ (abs(gold_R) >> 8);
    sum_R = sum_R+ (abs(o_R) >> 8);
    sum_Theta_gold = sum_Theta_gold+ abs(gold_Theta);
    sum_Theta = sum_Theta + abs(o_T);
*/


  bsg_wait_while(1);
}
