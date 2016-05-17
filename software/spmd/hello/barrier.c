#include "bsg_manycore.h"


int released = 0;
unsigned int barr[bsg_tiles_X*bsg_tiles_Y];
unsigned int barr3[bsg_tiles_X];	//Assumes a x dim = y dim


//Distributed Barrier
int barrier1(int x, int y, unsigned int * barr) 
{
  //Send barrier state updates to all cores
  bsg_volatile_access(barr[x + bsg_tiles_X*y]) += 1;
  for(int i = 0; i < bsg_tiles_X; i++) {
    for(int j = 0; j < bsg_tiles_Y; j++) {
      bsg_remote_store(i, j, barr + (x + bsg_tiles_X*y), barr[x + bsg_tiles_X*y]);
    }
  }

  for(int i = 0; i < bsg_tiles_X * bsg_tiles_Y; i++) {
    bsg_wait_while(bsg_volatile_access(barr[i]) != bsg_volatile_access(barr[x + bsg_tiles_X*y]));
  }
}


//Centralized Barrier
int barrier2(int x, int y, unsigned int * barr) 
{
  //Send barrier state updates to core 0
  bsg_remote_store(0, 0, barr + (x + bsg_tiles_X*y), 1);

  //Have core 0 check that it has received confirmation from all other cores
  if(x == 0 && y == 0) {
    for(int i = 0; i < bsg_tiles_X * bsg_tiles_Y; i++)
      bsg_wait_while(bsg_volatile_access(barr[i]) != 1);
   
    //If all cores have arrived, send out a release signal
    for(int i = 0; i < bsg_tiles_X; i++) {
      for(int j = 0; j < bsg_tiles_Y; j++) {
        bsg_remote_store(i, j, &released, 1);
      }
    }
  }

  //Wait until release, then set local release signal back to 0
  bsg_wait_while(bsg_volatile_access(released) != 1);
  bsg_volatile_access(released) = 0;
}


//Dependency Barrier
int barrier3(int x, int y, int * local_addr)
{
  //Completion Signals
  if(x == 0) {	//Pass up when row is confirmed
    for(int i = 1; i < bsg_tiles_X; i++) 
      bsg_wait_while(bsg_volatile_access(barr3[i]) == 0);
    
    bsg_remote_store(0, 0, &barr3[y], 1);

  } else {				//Pass left 
    bsg_remote_store(0, y, &barr3[x], 1);
  }


  //Propogate Release Signals

  //Have central core check for completion
  if(x == 0 && y == 0) {
    for(int i = 0; i < bsg_tiles_X; i++) 
      bsg_wait_while(bsg_volatile_access(barr3[i]) == 0);
    
    //Once all cores have arrived, send out release signal
    for(int i = 0; i < bsg_tiles_X; i++)
      bsg_remote_store(0, i, &released, 1); 
  }
  
  if(x == 0) {
    bsg_wait_while(bsg_volatile_access(released) == 0);
    
    for(int i = 1; i < bsg_tiles_X; i++)
      bsg_remote_store(i, y, &released, 1); 
    
  }

  //Release cores
  bsg_wait_while(bsg_volatile_access(released) != 1);
  released = 0;
}

