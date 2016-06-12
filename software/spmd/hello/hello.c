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



int main()
{
  int i=0,x=-1,lock_stat_ptr=0x8, lock_req_ptr=0xC, dummy;
  //volatile int * xp = (volatile int *) (0x80000001);
  //bsg_remote_int_ptr lock_req_ptr = bsg_remote_ptr(1,1,0xC);
  
  bsg_set_tile_x_y();

  bsg_remote_ptr_io_store(0,0x1260,bsg_x);
  bsg_remote_ptr_io_store(0,0x1264,bsg_y);

  bsg_remote_ptr_io_store(0,0x1234,0x13);

  //Core 0,0 sends a series of signals
  if ((bsg_x == 0) && (bsg_y == 0)){
    //bsg_remote_store(1,1,&data2,0xaaaaaaaa);
    //bsg_remote_store(1,1,&data2,0xbbbbbbbb);
    //bsg_remote_store(1,1,&data2,0xcccccccc);
    //bsg_remote_store(1,1,&data2,0xdddddddd);
    //bsg_remote_store(1,1,&data2,0xeeeeeeee);
    //Busy wait until outgoing signals decreases to 0
	
    bsg_remote_load(1,1,lock_req_ptr,x);
	x = 0xFF;
    while(x>3) { //get lock 0
      bsg_remote_load(0,0,lock_stat_ptr,x);
      bsg_remote_store(0,1,&data2,x);
    }
    bsg_remote_load(1,1,lock_req_ptr,x);
	x=0xFF;
    while(x>3) { //get lock 1
      bsg_remote_load(0,0,lock_stat_ptr,x);
      bsg_remote_store(0,1,&data2,x);
    }
    bsg_remote_load(1,1,lock_req_ptr,x);
	x=0xFF;
    while(x>3) { //get lock 2
      bsg_remote_load(0,0,lock_stat_ptr,x);
      bsg_remote_store(0,1,&data2,x);
    }
    bsg_remote_load(1,1,lock_req_ptr,x);
	x=0xFF;
    while(x>3) { //get lock 3
      bsg_remote_load(0,0,lock_stat_ptr,x);
      bsg_remote_store(0,1,&data2,x);
    }
    bsg_remote_load(1,1,lock_req_ptr,x);
	x=0xFF;
    while(x>0xF) { //fail to get lock
      bsg_remote_load(0,0,lock_stat_ptr,x);
      bsg_remote_store(0,1,&data2,x);
    }
	bsg_remote_load(1,1,0x20,x); //Release lock 2
    bsg_remote_load(1,1,lock_req_ptr,x);
	x=0xFF;
    while(x>3) {  //get lock 2
      bsg_remote_load(0,0,lock_stat_ptr,x);
      bsg_remote_store(0,1,&data2,x);
    } //finish
    bsg_remote_store(0,1,&data2,0xFF);
    bsg_finish();
  }
  //Have all other cores just generate traffic
  else {
    //while(1) bsg_remote_store(1,1,&data2,i++);
  }

  bsg_wait_while(1);
}