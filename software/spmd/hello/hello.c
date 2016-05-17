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

  // propagate across each row
  if ((bsg_x+1) != bsg_tiles_X)
  {
    bsg_remote_store(bsg_x+1,bsg_y,&bsg_x,bsg_x+1);
    bsg_remote_store(bsg_x+1,bsg_y,&bsg_y,bsg_y);
  }
}



int square(int c)
{
  return c*c;
}

//Simple handshake protocol. Only can have 1 outgoing packet to each core
int send(int x, int y, int * local_addr, int val, int src_x, int src_y) 
{
bsg_print_time();
  bsg_wait_while(bsg_volatile_access(received[x + bsg_tiles_X*y]) != 1);
  received[x + bsg_tiles_X*y] = 0;  
  int coreIndex = bsg_volatile_access(src_x) + bsg_tiles_X*bsg_volatile_access(src_y);
  bsg_remote_store(x, y, local_addr, val);
  bsg_remote_store(x, y, sent + coreIndex, 1); 
bsg_print_time();
}

//Should we specify who to receive from?
int receive()
{
  bsg_volatile_access(x_index);
  bsg_volatile_access(y_index);

  while(1) {
    for(int i = 0; i < bsg_tiles_X; i++, x_index = (x_index+1)%bsg_tiles_X) {
      for(int j = 0; j < bsg_tiles_Y; j++, y_index = (y_index+1)%bsg_tiles_Y) {
        if(bsg_volatile_access(sent[x_index+y_index*bsg_tiles_X]) == 1) {
          bsg_remote_store(x_index, y_index, received[bsg_volatile_access(bsg_x) + bsg_tiles_X*bsg_volatile_access(bsg_y)], 1);
	  sent[x_index+y_index*bsg_tiles_X] = 0;
          bsg_print_time();
 	  return 1;
        }
      }
    }
  }
  return 0;
}


int main()
{
  bsg_set_tile_x_y();

  bsg_remote_ptr_io_store(0,0x1260,bsg_x);
  bsg_remote_ptr_io_store(0,0x1264,bsg_y);

  bsg_remote_ptr_io_store(0,0x1234,0x13);
  
  //Core (1,1) sends to core (0,0)  
  if(bsg_volatile_access(bsg_x) == 1 && bsg_volatile_access(bsg_y) == 1) {
    send(0, 0, &data, data, bsg_volatile_access(bsg_x), bsg_volatile_access(bsg_y));
  }

  //Core (0,0) waits until data is received
  if(bsg_x == 0 && bsg_y == 0) {
    receive();
  }
  

  //Barrier to prevent an early finish from occurring.
  barrier3(bsg_x, bsg_y, barr);

  if ((bsg_x == bsg_tiles_X-1) && (bsg_y == bsg_tiles_Y-1))
    bsg_finish();

  bsg_wait_while(1);
}

