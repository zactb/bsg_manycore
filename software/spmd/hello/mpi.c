#include "bsg_manycore.h"


int RR_index;
int x_index = 0;
int y_index = 0;
int sent[bsg_tiles_X*bsg_tiles_Y];
int received[bsg_tiles_X*bsg_tiles_Y];
int count[bsg_tiles_X*bsg_tiles_Y];
int inc = 0;
int data = 0;

//Note, have to go back and initialize received array to 1
/*
//Simple handshake protocol. Only can have 1 outgoing packet to each core
int send1(int x, int y, int * local_addr, int val, int src_x, int src_y) 
{
bsg_print_time();
  bsg_wait_while(bsg_volatile_access(received[x + bsg_tiles_X*y]) != 1);
  received[x + bsg_tiles_X*y] = 0;  
  int coreIndex = bsg_volatile_access(src_x) + bsg_tiles_X*bsg_volatile_access(src_y);
  bsg_remote_store(x, y, local_addr, val);
  bsg_remote_store(x, y, &sent[coreIndex], 1); 
bsg_print_time();
}


int send2(int x, int y, int * local_addr, int val) 
{
  //Wait until we have more counts
  bsg_wait_while(bsg_volatile_access(count[x + bsg_tiles_X*y]) <= 0 && bsg_volatile_access(inc) != 1);
  count[x + bsg_tiles_X*y] += inc;
  inc = 0;
 
  bsg_remote_store(x, y, local_addr, val);
  bsg_remote_store(x, y, &received, 1);
  count[x + bsg_tiles_X*y] -= 1;  

  return 1;
}


int receive1()
{
  bsg_volatile_access(x_index);
bsg_print_time();
  bsg_volatile_access(y_index);
  while(1) {
    while(1) {
      if(bsg_volatile_access(sent[x_index+y_index*bsg_tiles_X]) == 1) {
        bsg_remote_store(x_index, y_index, received[x_index+y_index*bsg_tiles_X], 1);
	sent[x_index+y_index*bsg_tiles_X] = 0;
bsg_print_time();
 	return 1;
      }
      y_index = (y_index+1)%bsg_tiles_Y;
    }
    x_index = (x_index+1)%bsg_tiles_X;
  }
  return 0;
}*/
/*
//Halts the core and waits for receival of data from specified core?
int receive2() 
{
  int index = bsg_volatile_access(bsg_x) + bsg_tiles_X*bsg_volatile_access(bsg_y);
  
  //Iterate through buffers with RR and find next message to receive
  while(bsg_volatile_access(received[RR_index]) != 1) {
    RR_index = (RR_index+1)%(bsg_tiles_X*bsg_tiles_Y);
  } 
  
  received[RR_index] = 
*/
/*
We need to know how many have arrived
*/

  //return 1;
//}


