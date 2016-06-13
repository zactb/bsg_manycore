
#include "bsg_manycore.h"
#include "barrier.c"
#include "phasedetector.h"
#include "fir.c"
//#include "mpi.c"
// these are private variables
// we do not make them volatile
// so that they may be cached


int bsg_x = -1;
int bsg_y = -1;
#define NUM_FIR  5
#define NUM_M  10
#define NUM_COR  1
#define LMSize  (NUM_M * 3 * 4)
#define RESPSIZE ((NUM_FIR * 4) + 1) 

//int RR_index;
int x_index = 0;
int y_index = 0;
int sent[bsg_tiles_X*bsg_tiles_Y];
int received[bsg_tiles_X*bsg_tiles_Y];
int LMSpace[LMSize];
		    

//int count[bsg_tiles_X*bsg_tiles_Y];
//int inc = 0;
int data = 0;
int curOffset = 0;
int bsg_remote_read2(int x, int y, int addr );


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

void phase_detector_init(){
  //need to let everyone know where they should be storing 
  int i, j, ii, jj;
  for (i = 0; i < LMSize; i++)
    LMSpace[i] = 0;
  for(i = 0; i < bsg_tiles_X; i++){
    for (j = 0; j < bsg_tiles_Y; j++){
      bsg_remote_store(i, j, bsg_y*bsg_tiles_X + bsg_x, &LMSpace);
    }
  }

  //some barrier to wait for everyone to finish this
}

int bsg_lock_release(int x, int y, int locknum){
  //do I need to make this non 0-3
  return *(int*) ( (0x3 << 30)
  	           | ((y) << (30 - (bsg_noc_ybits)))
		   | ((x) << (30 - bsg_noc_xbits -bsg_noc_ybits))
		   | ((int) (locknum << 4)));
		   
}

int bsg_outstanding_stores(){
  return *(int*) ( (int) (0x3 << 30)
  		   | (int) (0x4));
}

int bsg_lock_request(int x, int y){
  return *(int*) ( (0x3 << 30)
  	           | ((y) << (30 - (bsg_noc_ybits)))
		   | ((x) << (30 - bsg_noc_xbits -bsg_noc_ybits))
		   | ((int) (0xC)));
}

int bsg_lock_status(int x, int y, int locknum){
  return *(int*) ( (0x3 << 30)
  	           | ((y) << (30 - (bsg_noc_ybits)))
		   | ((x) << (30 - bsg_noc_xbits -bsg_noc_ybits))
		   | ((int) (0x8)));
}


int bsg_remote_read2(int x, int y, int addr ){
  return *(int*) ( (0x3 << 30)
                 | ((x) << (30 - bsg_noc_xbits)) 
		 | ((y) << (30 - bsg_noc_xbits-bsg_noc_ybits))
		 | ((int) (addr) )
		 );
}


void phase_detector(){
  phase_detector_init();
  int i, j;
  int val_1 = 0;
  int val_2 = 0;
  int val_3 = 0;
  int val_4 = 0;
  int tar_x, tar_y;
  int index_m;
  int running = 1;
  int response[RESPSIZE];
  int callCordic = 0;
  int cin1, cin2, fin1, fin2;
  int active[NUM_FIR];
  int addr;



  /* TODO 
    What to set registers to by default
    What is register set to after releasing the lock
  */
  val_1 = -65536;
  val_2 = 65536;

  for (i = 0; i < RESPSIZE; i++)
    response[i] = 0xE;
  for (i = 0; i < NUM_FIR; i++)
    active[i] = 1;



  /*
    LMspace for masters: each one can receive from up to 4 FIRs and 1 Cordic. Each response has 3 values.
  */
  if ( (bsg_y == 3) || ((bsg_x == 3) && (bsg_y == 2))){
    bsg_print_time();
    //FIR
    /*
      get 2 values from master
      write 2 back to master
    */
    
    while(1){
      for (i = 0; i < NUM_M; i = i++) {
        tar_x = i/bsg_tiles_X;
	tar_y = i%bsg_tiles_X;
	addr = *(int*)(tar_y*bsg_tiles_X + tar_x);
	int k;
	for (j = 0; j < 4; j++){
	  k = i*12 + j*3;
          if ( LMSpace[k] != 0){ //someone wrote something
	    int fv1, fv2;
	    fir ( LMSpace[k+1], LMSpace[k+2], &fv1, &fv2, LMSpace[k] );
	    // they should only store to indexes 0-23 ( master receives up to 3 values from 8 different FIR sources) 
	    bsg_remote_store( tar_x, tar_y, addr + (bsg_y*bsg_tiles_X + bsg_x)*4*3 + 3*LMSpace[k] + 1, fv1);
	    bsg_remote_store( tar_x, tar_y, addr + (bsg_y*bsg_tiles_X + bsg_x)*4*3 + 3*LMSpace[k] + 2, fv2);//pretty sure this does nothing
	    bsg_remote_store( tar_x, tar_y, addr + (bsg_y*bsg_tiles_X + bsg_x)*4*3 + 3*LMSpace[k], 1);
	    bsg_volatile_access(LMSpace[j]) = 0;
          }
	}
      }
    }

  }
  /*
    LMspace for masters: each one can receive from up to 4 FIRs and 1 Cordic. Each response has 3 values.
  */
  else if ((bsg_y == 2) && (bsg_x == 2)) {
    bsg_print_time();
    //cordicc
    /*
      get 4 values from master
      write 2 back to master
    */
    while(1){
      for (int i = 0; i < NUM_M; i = i++) {
        tar_x = i/bsg_tiles_X;
	tar_y = i%bsg_tiles_X;
	addr = *(int*)(tar_y*bsg_tiles_X + tar_x);
        if ( LMSpace[i*3] != 0){ //someone wrote something
	  int cv1, cv2;
	  cordiccart2pol2 ( LMSpace[i*3+1], LMSpace[i*3+2], &cv1, &cv2 )
	  bsg_remote_store( tar_x, tar_y, addr + NUM_FIR*3*4, cv1);
	  bsg_remote_store( tar_x, tar_y, addr + NUM_FIR*3*4, cv2);
	  bsg_remote_store( tar_x, tar_y, addr + NUM_FIR*3*4, LMSpace[i*3]);
	  /*
	  bsg_remote_store( (i/5)%bsg_tiles_X, (i/5)/bsg_tiles_X, &LMSpace[25], cv1);
	  bsg_remote_store( (i/5)%bsg_tiles_X, (i/5)/bsg_tiles_X, &LMSpace[26], cv2);
	  bsg_remote_store( (i/5)%bsg_tiles_X, (i/5)/bsg_tiles_X, &LMSpace[24], 1);
	  */
          bsg_print_time(); 
	  bsg_volatile_access(LMSpace[i*3]) = 0;
        }
      }
    }
  }

  /*
    LMspace each of the 5 Masters can request up to 4 FIRs. Each request gives 3 values.
    LMSpace: Cordicc can receive up to 4 requests from 5 different masters. Each request gives 3 values
  */
  else{
    bsg_print_time();
    //master
    /*
      write 2 values to fir of choice (how do I decide this??)
      get 2 back
      write 4 values to cordicc
      get 2 back
    */
    /* will this work with just send/receive?? */
    
    tar_x = 2;
    tar_y = 1;

    while (1 /* words left */){
      //fir requests
      index_m = 4*3*(bsg_y*bsg_tiles_X + bsg_x); //part of offset for writing into target
      for (i = 0; i < NUM_FIR; i++) {
        if (i == 0){
	  fin1 = val_1;
	  fin2 = val_2;
	}
	else{
	  fin1 = val_3;
	  fin2 = val_4;
	}
	if (active[i] <=4){
	  for (j = 0; j < 4; j++) {
	    tar_x = (i+3)%4;
	    tar_y = i ? 3 : 2;
	    addr = (*(int*)(4*3 + ((i+3)%4)));
	    if (bsg_volatile_access(response[4*i+j]) == 0xF){
	      response[4*i+j] = bsg_lock_request( tar_x, tar_y); 
		
	    //  active = active+1;
	    }
	    /*
	    if (bsg_volatile_access(response[4*i+j]) == 0xF){
	      response[4*i+j] = bsg_lock_request(2, i);
	    }
	    */
            if (bsg_volatile_access(response[4*i+j]) <= 3 ) {
	      bsg_remote_store(tar_x, tar_y, addr + index_m+3*response[4*i+j]+1, fin1);
	      bsg_remote_store(tar_x, tar_y, addr + index_m+3*response[4*i+j]+2, fin2);
	      bsg_remote_store(tar_x, tar_y, addr + index_m+3*response[4*i+j], active[i]);
	      /*
              bsg_remote_store( i, 2, &LMSpace[index_m + 3*response[4*i+j]+1], fin1);
              bsg_remote_store( i, 2, &LMSpace[index_m + 3*response[4*i+j] +2], fin2);
              bsg_remote_store( i, 2, &LMSpace[index_m + 3*response[4*i+j]], active[i]);
	      */
	      response[4*i+j] = 4; //to prevent it from repeatedly sending?
	      active[i] = active[i] +1;
	    }
          }
	}
      }

      //need to check for fir output
      if (!callCordic ){
        for (i = 0; i < NUM_FIR; i++){
	  j = i*NUM_FIR;
          if (  (bsg_volatile_access( LMSpace[i] ) == 1)
	     && (bsg_volatile_access( LMSpace[i+3] ) == 1)
	     && (bsg_volatile_access( LMSpace[i+6] ) == 1)
	     && (bsg_volatile_access( LMSpace[i+9] ) == 1)){


	    callCordic = 1;
	    cin1 = LMSpace[i+1] + LMSpace[i+9+1];
	    cin2 = LMSpace[i+3+1] - LMSpace[i+6+1];
	    LMSpace[i] = 0;
	    LMSpace[i+3] = 0;
	    LMSpace[i+6] = 0;
	    LMSpace[i+9] = 0;
	    //lock release
	    response[i*4] = 0xF;
	    response[i*4+1] = 0xF;
	    response[i*4+2] = 0xF;
	    response[i*4+3] = 0xF;
	    bsg_lock_release(2, 2, 0x0);
	    bsg_lock_release(2, 2, 0x4);
	    bsg_lock_release(2, 2, 0x8);
	    bsg_lock_release(2, 2, 0xC);
	    active[i] = 1;
	  }
	}
      }
      /*
      if (!callCordic){
        for (i = 0; i < 8; i++) {
          if ( bsg_volatile_access(LMSpace[3*i]) == 1 ){
	    callCordic = 1;
	    cord_In_Index = 3*i;
	  }
        }
      }
      */
      
      if (callCordic){
	//cordic is fortunately 2, 2
        addr = (*(int*)(10));
	if ( bsg_volatile_access( response[RESPSIZE-1] ) == 0xF ){
	  response[RESPSIZE-1] = bsg_lock_request(2, 2);
	}
	if ( bsg_volatile_access( response[RESPSIZE-1] ) <= 3 ){
            bsg_remote_store( 2, 2, addr + 3*(bsg_y*bsg_tiles_X + bsg_x) +1, cin1 );
            bsg_remote_store( 2, 2, addr + 3*(bsg_y*bsg_tiles_X + bsg_x) +2, cin2 );
            bsg_remote_store( 2, 2, addr + 3*(bsg_y*bsg_tiles_X + bsg_x), cin1 );
	    /*
            bsg_remote_store( 2, 2, addr + 3*response[RESPSIZE-1]+1, cin1 );
            bsg_remote_store( 2, 2, addr + 3*response[RESPSIZE-1]+2, cin2 );
            bsg_remote_store( 2, 2, addr + 3*response[RESPSIZE-1], 1);
	    */
	}
      }

      //received from cordic; start processing and allow for more cordic calls
      if ( callCordic && (bsg_volatile_access( LMSpace[NUM_FIR*4*3] ) != 0) ){
        //do some processing with LMSpace[25] and LMspace[26]
	LMSpace[NUM_FIR*4*3+1] = 0;
	LMSpace[NUM_FIR*4*3+2] = 0;
	LMSpace[NUM_FIR*4*3] = 0;
	bsg_volatile_access(response[RESPSIZE-1]);
	//release lock
	bsg_lock_release(2, 2, response[RESPSIZE-1]);
	response[RESPSIZE-1] = 0xF;
	callCordic = 0;
      }


    }
    /*
    bsg_remote_store( tar_x, tar_y, *(char*)(curOffset + index_m) + 3*(bsg_x*bsg_tiles_Y + bsg_y), val_1);
    bsg_remote_store( tar_x, tar_y, *(char*)(curOffset + index_m) + 3*(bsg_x*bsg_tiles_Y + bsg_y)+1, val_2);
    bsg_remote_store( tar_x, tar_y, *(char*)(curOffset + index_m) + 3*(bsg_x*bsg_tiles_Y + bsg_y)+2, 1);
    */
    
  }
}


int main()
{
  bsg_set_tile_x_y();

  bsg_remote_ptr_io_store(0,0x1260,bsg_x);
  bsg_remote_ptr_io_store(0,0x1264,bsg_y);


  bsg_remote_ptr_io_store(0,0x1234,0x13);

  
  /*
  //Core (1,1) sends to core (0,0)  
  if(bsg_volatile_access(bsg_x) == 1 && bsg_volatile_access(bsg_y) == 1) {
    send(0, 0, &data, data, bsg_volatile_access(bsg_x), bsg_volatile_access(bsg_y));
  }

  //Core (0,0) waits until data is received
  if(bsg_x == 0 && bsg_y == 0) {
    receive();
  }
  */
  

  //Barrier to prevent an early finish from occurring.
  //barrier3(bsg_x, bsg_y, barr);


  if ((bsg_x == bsg_tiles_X-1) && (bsg_y == bsg_tiles_Y-1))
    bsg_finish();

  bsg_wait_while(1);
}

