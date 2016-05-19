module bsg_manycore_tile

import bsg_vscale_pkg::*
       , bsg_noc_pkg::*; // {P=0, W, E, N, S}

 #( parameter dirs_p            = 4
   ,parameter stub_p            = {dirs_p{1'b0}} // {s,n,e,w}
   ,parameter x_cord_width_p       = 5
   ,parameter y_cord_width_p       = 5 

   ,parameter bank_size_p       = -1
   ,parameter num_banks_p       = -1
   ,parameter data_width_p      = hdata_width_p
   ,parameter addr_width_p      = haddr_width_p
   ,parameter mem_addr_width_lp = $clog2(num_banks_p) + `BSG_SAFE_CLOG2(bank_size_p)
   ,parameter packet_width_lp   = 6 + x_cord_width_p + y_cord_width_p
                                    + x_cord_width_p + y_cord_width_p
                                    + addr_width_p + data_width_p
   ,parameter ret_pack_width_lp = 1 + x_cord_width_p + y_cord_width_p + 1 + 1 + 1 + 1
   ,parameter debug_p = 0
  )
  ( input                                       clk_i
   ,input                                       reset_i

   // input fifos
   ,input   [dirs_p-1:0] [packet_width_lp-1:0]  data_i
   ,input   [dirs_p-1:0]                        v_i
   ,output  [dirs_p-1:0]                        ready_o
   
   ,input   [dirs_p-1:0] [ret_packet_width_lp-1:0]  ret_data_i
   ,input   [dirs_p-1:0]                        ret_v_i
   ,output  [dirs_p-1:0]                        ret_ready_o

   // output channels
   ,output  [dirs_p-1:0] [packet_width_lp-1:0]  data_o
   ,output  [dirs_p-1:0]                        v_o
   ,input   [dirs_p-1:0]                        ready_i

   ,output  [dirs_p-1:0] [ret_packet_width_lp-1:0]  ret_data_o
   ,output  [dirs_p-1:0]                        ret_v_o
   ,input   [dirs_p-1:0]                        ret_ready_i

   // tile coordinates
   ,input   [x_cord_width_p-1:0]                 my_x_i
   ,input   [y_cord_width_p-1:0]                 my_y_i

  );

   logic [packet_width_lp-1:0] proc_to_router_data, router_to_proc_data;
   logic                       proc_to_router_ready, router_to_proc_ready,
							   proc_to_router_v, router_to_proc_v;

   logic [ret_packet_width_lp-1:0] ret_proc_to_router_data, ret_router_to_proc_data;
   logic                       ret_proc_to_router_ready, ret_router_to_proc_ready,
							   ret_proc_to_router_v, ret_router_to_proc_v;
   
   bsg_mesh_router_buffered #(.width_p(packet_width_lp)
                              ,.x_cord_width_p(x_cord_width_p)
                              ,.y_cord_width_p(y_cord_width_p)
                              ,.debug_p(debug_p)
                              // adding proc into stub
                              ,.stub_p({stub_p, 1'b0})
                              ) bmrb
     (.clk_i   (clk_i)
      ,.reset_i(reset_i)
      ,.v_i     ({ v_i,     proc_to_router_v}    )
      ,.data_i  ({ data_i,  proc_to_router_data })
      ,.ready_o ({ ready_o, proc_to_router_ready})

      ,.v_o     ({ v_o,     router_to_proc_v}    )
      ,.data_o  ({ data_o,  router_to_proc_data} )
      ,.ready_i ({ ready_i, router_to_proc_ready})
	  
      ,.my_x_i
      ,.my_y_i
      );
	  
    /* TODO
	 * Add an operation(6'h3F?) to check if remote stores are complete. 
	 *  -For the time being, I have removed error checking for remote loads.
	 *   All remote loads will return the status of the outstanding stores counter.
	 *   For this operation, since other remote loads are not yet implemented, 
	 *   it should be safe to bypass the memory banked crossbar.
	 * Make sure we don't add to store counter for peripherals!
	 * 	-this  means checking stores against column/row0 and num_tiles_{y|x}_p-1?
	 *  -No? there must be a different command, as col0 IS col0. Maybe col/row =numtiles?
	 *  -After investigation, I found that there is only one opcode for remote stores, and
	 *   it cannot have -1 or >width/height address, so we assume for now that a separate
	 *   command would have to be implemented for peripherals. In the future, updating
	 *   the  logic for the counter would be necessary.
	 */
   bsg_mesh_router_buffered #(.width_p(ret_packet_width_lp) //make this packet, just make widths=1
                              ,.x_cord_width_p(x_cord_width_p)
                              ,.y_cord_width_p(y_cord_width_p)
                              ,.debug_p(debug_p) 
                              // adding proc into stub
                              ,.stub_p({stub_p, 1'b0}) //keep as-is
                              ) ret_rtr
     (.clk_i   (clk_i)
      ,.reset_i(reset_i)
      ,.v_i     ({ ret_v_i, ret_proc_to_router_v}) //valid signals from each direction
      ,.data_i  ({ ret_data_i,  ret_proc_to_router_data }) //packet (addr+data+from not needed,  can expand functionality later)
      ,.ready_o ({ ret_ready_o, ret_proc_to_router_ready}) //ready to each direction

      ,.v_o     ({ ret_v_o, ret_router_to_proc_v}    ) //valid to each dir 
      ,.data_o  ({ ret_data_o,  ret_router_to_proc_data} ) //data out
      ,.ready_i ({ ret_ready_i, ret_router_to_proc_ready}) //ready from each dir

      ,.my_x_i
      ,.my_y_i
      );

   logic 		       freeze;


   bsg_manycore_proc #(
                       .x_cord_width_p (x_cord_width_p)
                       ,.y_cord_width_p(y_cord_width_p)
                       ,.debug_p       (debug_p)
                       ,.bank_size_p   (bank_size_p)
                       ,.num_banks_p   (num_banks_p)
                       ,.data_width_p  (data_width_p)
                       ,.addr_width_p  (addr_width_p)
                       ) proc
   (.clk_i   (clk_i)
    ,.reset_i(reset_i)
	
    ,.v_i    (router_to_proc_v)
    ,.data_i (router_to_proc_data)
    ,.ready_o(router_to_proc_ready)
    ,.v_o    (proc_to_router_v)
    ,.data_o (proc_to_router_data)
    ,.ready_i(proc_to_router_ready)	
	
    ,.ret_v_i    (ret_router_to_proc_v)
    ,.ret_data_i (ret_router_to_proc_data)
    ,.ret_ready_o(ret_router_to_proc_ready)
    ,.ret_v_o    (ret_proc_to_router_v)
    ,.ret_data_o (ret_proc_to_router_data)
    ,.ret_ready_i(ret_proc_to_router_ready)
	
    ,.my_x_i(my_x_i)
    ,.my_y_i(my_y_i)

    ,.freeze_o(freeze)
    );


endmodule