#include "phasedetector.h"
#include "bsg_manycore.h"
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

static const int Kvalues[MAX_ITER] = { 	11863283, 10610843, 10294029, 10214538,
				10194646, 10189671, 10188428, 10188117,
				10188039, 10188020, 10188015, 10188014,
				10188013, 10188013, 10188013, 10188013,
				10188013, 10188013, 10188013, 10188013,
				10188013, 10188013, 10188013, 10188013 };
static const int angles[MAX_ITER] = {	13176794, 	7778716, 	4110059, 	2086330, 	
				1047213,	524117,		262122,		131069,		
				65535,		32767,		16383,		8191,		
				4095,		2047,		1023,		511,		
				255,		127,		64,		31,
				16,		8,		4,		2};

static const int PI = 52707178;




int main()
{
  int i=0,lock_stat_ptr=0x8, lock_req_ptr=0xC, dummy;
  //volatile int * xp = (volatile int *) (0x80000001);
  //bsg_remote_int_ptr lock_req_ptr = bsg_remote_ptr(1,1,0xC);
  
  bsg_set_tile_x_y();

  bsg_remote_ptr_io_store(0,0x1260,bsg_x);
  bsg_remote_ptr_io_store(0,0x1264,bsg_y);

  bsg_remote_ptr_io_store(0,0x1234,0x13);


/*
 * Put cordiccart2pol code here
 */

  //Tell the other cores the addresses of my inputs x and y
  int input[4][4];		// [4 locks] * [valid, x, y, ret_addr]
  int tileNum = bsg_volatile_access(bsg_x) + bsg_volatile_access(bsg_y)*bsg_tiles_X;
  for(int index = 0; index < bsg_tiles_X*bsg_tiles_Y; index++) {
    bsg_remote_store(index%bsg_tiles_X, index/bsg_tiles_X, (int *)(tileNum << 2), input);
  }
 
  barrier3(bsg_x, bsg_y, barr);
 
  while(1) {
    //Idle until someone claims it
    for(int j = 0; j < 4; j++) {
      if(input[j][0] == 1) {

        int x = input[j][1];
        int y = input[j][2];
        int r = 0;
        int theta = 0;

        int x_sh, y_sh;

        //if in 2nd/3rd quadrants, invert
        theta = x < 0 ? (y < 0 ? (int)(theta - PI) : (int)(theta + PI)) : (int)theta;
        y = x < 0 ? (int)-y : y;
        x = x < 0 ? (int)-x : x;

        //apply rotations of <1, (+/-)0.5^i> to approach 0
        for(i=0; i<NO_ITER; i++) {
          x_sh = x>>i;
          y_sh = y>>i;
          theta = y < 0 ? theta - angles[i] : theta + angles[i];
          x = y < 0 ? (int)(x-y_sh) :(int) (x+y_sh); 	
          y = y < 0 ? (int)(y+x_sh) :(int) (y-x_sh);
        }

        //adjust radius for expansion
        int xl = (x << 16) >> 16;
        int kl = (Kvalues[NO_ITER-1] << 16) >> 16;
        int xh = x >> 16;
        int kh = Kvalues[NO_ITER-1] >> 16;
  
        int temp = ((xl*kl) >> 24) + (((xh*kl)+(xl*kh)) >> 8) + ((xh*kh) << 8);
        r = temp;

        int * sent = (int *)*(int *)(input[j][3]);
        //Send the results
        bsg_remote_store(input[j][3]/bsg_tiles_X, input[j][3]/bsg_tiles_Y, sent, r);
        bsg_remote_store(input[j][3]/bsg_tiles_X, input[j][3]/bsg_tiles_Y, sent+1, theta);
        bsg_remote_store(input[j][3]/bsg_tiles_X, input[j][3]/bsg_tiles_Y, received, 1);
      }
    }
  }
}



