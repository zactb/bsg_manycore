`include "bsg_manycore_packet.vh"

module bsg_manycore_proc #(x_cord_width_p   = "inv"
                           , y_cord_width_p = "inv"
                           , data_width_p   = 32
                           , addr_width_p   = 32
                           , ret_packet_width_lp = 5 + y_cord_width_p + x_cord_width_p//`bsg_manycore_packet_width(1,1,x_cord_width_p,y_cord_width_p)
                           , packet_width_lp = `bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p)

                           , debug_p        = 0
                           , bank_size_p    = "inv" // in words
                           , num_banks_p    = "inv"

                           // this is the size of the receive FIFO
                           , proc_fifo_els_p = 4
                           , mem_width_lp    = $clog2(bank_size_p) + $clog2(num_banks_p)
                           )
   (input   clk_i
    , input reset_i

    , input v_i
    , input [packet_width_lp-1:0] data_i
    , output ready_o

    , output v_o
    , output [packet_width_lp-1:0] data_o
    , input ready_i
	
    , input ret_v_i
    , input [ret_packet_width_lp-1:0] ret_data_i
    , output ret_ready_o

    , output ret_v_o
    , output [ret_packet_width_lp-1:0] ret_data_o
    , input ret_ready_i

    // tile coordinates 
    , input   [x_cord_width_p-1:0]                 my_x_i
    , input   [y_cord_width_p-1:0]                 my_y_i

    , output logic freeze_o
    );

   // pkt from coords
   logic  [x_cord_width_p-1:0] from_x_cord;
   logic  [y_cord_width_p-1:0] from_y_cord;
   
   //Quick implementation of ourstanding store counter:
   localparam str_cntr_wid_lp = 16;
   assign ret_ready_o = 1'b1;// Always accepts incoming messages.
   logic out_store_v = v_o; //TODO: ensure signal is only node-to-node remote stores, no peripherals.
   logic [str_cntr_wid_lp:0] out_stores; //minimum width is ceil(log(num cores * 2 directions * pipeline depth))
   logic ret_store_cntr; //returns the store counter on a remote load.

   always_ff @(posedge clk_i) begin
       if(~(out_store_v^ret_v_i)) begin //both or neither
	   out_stores <= out_stores;
       end else if (out_store_v) begin
	   out_stores <= out_stores+1;
       end else if (ret_v_i) begin
	   out_stores <= (out_stores==0) ? 0 : out_stores-1;
           if(out_stores==0) $display("ERROR: NEGATIVE OUTSTANDING STORE COUNTER AT NODE: x%x y%x",my_x_i,my_y_i);
       end
   end
	
   // input fifo from network

   logic cgni_v, cgni_yumi, ret_cgni_v;
   logic [packet_width_lp-1:0] cgni_data;
   
   //Do not store if we can't send a return message.
   assign ret_cgni_v = cgni_v & ret_ready_i;

   // this fifo buffers incoming remote store requests
   // it is a little bigger than the standard twofer to accomodate
   // bank conflicts

   bsg_fifo_1r1w_small #(.width_p(packet_width_lp)
                        ,.els_p (proc_fifo_els_p)
                        ) cgni
     (.clk_i   (clk_i  )
      ,.reset_i(reset_i)

      ,.v_i     (v_i    )
      ,.data_i  (data_i )
      ,.ready_o (ready_o)

      ,.v_o    (cgni_v   )
      ,.data_o (cgni_data)
      ,.yumi_i (cgni_yumi)
      );


   // decode incoming packet
   logic                       pkt_freeze, pkt_unfreeze, pkt_remote_store, pkt_unknown;
   logic [data_width_p-1:0]    remote_store_data;
   logic [(data_width_p>>3)-1:0] remote_store_mask;
   logic [addr_width_p-1:0]    remote_store_addr;
   logic                       remote_store_v, remote_store_yumi;

   if (debug_p)
   always_ff @(negedge clk_i)
     if (v_o)
       $display("%m attempting remote store of data %x, ready_i = %x",data_o,ready_i);

   if (debug_p)
     always_ff @(negedge clk_i)
       if (ret_cgni_v)
         $display("%m data %x avail on cgni (cgni_yumi=%x,remote_store_v=%x, remote_store_addr=%x, remote_store_data=%x, remote_store_yumi=%x)",cgni_data,cgni_yumi,remote_store_v,remote_store_addr, remote_store_data, remote_store_yumi);

   bsg_manycore_pkt_decode #(.x_cord_width_p (x_cord_width_p)
                             ,.y_cord_width_p(y_cord_width_p)
                             ,.data_width_p  (data_width_p )
                             ,.addr_width_p  (addr_width_p )
                             ) pkt_decode
     (.v_i                 (ret_cgni_v)
     //(.v_i                 (cgni_v)
      ,.data_i             (cgni_data)
      ,.pkt_freeze_o       (pkt_freeze)
      ,.pkt_unfreeze_o     (pkt_unfreeze)
      ,.pkt_unknown_o      (pkt_unknown)

      ,.pkt_remote_store_o (remote_store_v)
      ,.data_o             (remote_store_data)
      ,.addr_o             (remote_store_addr)
      ,.from_y_cord_o      (from_y_cord)
      ,.from_x_cord_o      (from_x_cord)
      ,.mask_o             (remote_store_mask)
      );

   // deque if we successfully do a remote store, or if it's
   // either kind of packet freeze instruction
   assign cgni_yumi = remote_store_yumi | pkt_freeze | pkt_unfreeze;

   // create freeze gate
   logic                       freeze_r;
   assign freeze_o = freeze_r;

   always_ff @(posedge clk_i)
     if (reset_i)
       freeze_r <= 1'b1;
     else
       if (pkt_freeze | pkt_unfreeze)
         begin
            $display("## freeze_r <= %x",pkt_freeze);
            freeze_r <= pkt_freeze;
         end

   logic [1:0]                  core_mem_v;
   logic [1:0]                  core_mem_w;
   logic [1:0] [addr_width_p-1:0] core_mem_addr;
   logic [1:0] [data_width_p-1:0] core_mem_wdata;
   logic [1:0] [(data_width_p>>3)-1:0] core_mem_mask;
   logic [1:0]                         core_mem_yumi;
   logic [1:0]                         core_mem_rv;
   logic [1:0] [data_width_p-1:0]      core_mem_rdata;

   logic [data_width_p-1:0] muxed_core_mem_rdata = ret_store_cntr ? 
                                                   { {(data_width_p - str_cntr_wid_lp){1'b0}}, out_stores} : 
                                                   core_mem_rdata[1];
   logic core_mem_reservation_r;

   logic [addr_width_p-1:0]      core_mem_reserve_addr_r;

   // implement LR (load word reserved)
   always_ff @(posedge clk_i)
     begin
        // if we commit a reserved memory access
        // to the interface, then the reservation takes place
        if (core_mem_v & core_mem_reserve_1 & core_mem_yumi[1])
          begin
             // copy address
             core_mem_reservation_r  <= 1'b1;
             core_mem_reserve_addr_r <= core_mem_addr[1];
	     $display("## x,y = %d,%d enabling reservation on %x",my_x_i,my_y_i,core_mem_addr[1]);
          end
        else
          // otherwise, we clear existing reservations if the corresponding
          // address is committed as a remote store
          begin
             if (remote_store_v && (core_mem_reserve_addr_r == remote_store_addr) && remote_store_yumi)
	       begin
		  core_mem_reservation_r  <= 1'b0;
		  $display("## x,y = %d,%d clearing reservation on %x",my_x_i,my_y_i,core_mem_reserve_addr_r);
	       end
          end
     end

   bsg_vscale_core #(.x_cord_width_p (x_cord_width_p)
                     ,.y_cord_width_p(y_cord_width_p)
                     )
            core
     ( .clk_i   (clk_i)
       ,.reset_i (reset_i)
       ,.freeze_i (freeze_r)

       ,.m_v_o        (core_mem_v)
       ,.m_w_o        (core_mem_w)
       ,.m_addr_o     (core_mem_addr)
       ,.m_data_o     (core_mem_wdata)
       ,.m_reserve_1_o  (core_mem_reserve_1)
       ,.m_reservation_i(core_mem_reservation_r)
       ,.m_mask_o    (core_mem_mask)

       // for data port (1), either the network or the banked memory can
       // deque the item.
	   //TODO: Added some hacky method of returning the outstanding stores counter
	   //At the same time as yumi. Based on my understanding of the vscale core,
	   //the remote load cannot be waiting on any other loads, so it's safe to bypass
	   //the mem_banked_crossbar. If this is not the case, we will see it in sim.
       //,.m_yumi_i    ({(v_o & ready_i) | core_mem_yumi[1] | ret_store_cntr
       ,.m_yumi_i    ({(v_o & ready_i) | core_mem_yumi[1]
                       , core_mem_yumi[0]})
       //,.m_v_i       ({core_mem_rv[1] | ret_store_cntr, core_mem_rv[0]})
       ,.m_v_i       (core_mem_rv)
       //,.m_data_i    ( {muxed_core_mem_rdata, core_mem_rdata[0]} )
       ,.m_data_i    ( core_mem_rdata)
       ,.my_x_i (my_x_i)
       ,.my_y_i (my_y_i)
       );
   always_ff @(negedge clk_i) 
     if(core_mem_rv[1] & ret_store_cntr) 
	   $display("IMPLEMENTATION ERROR: CANNOT BYPASS CROSSBAR FOR REMOTE LOADS (BARRIER ON OUTSTANDING LOADS)");	   
	   
   bsg_manycore_pkt_encode #(.x_cord_width_p (x_cord_width_p)
                             ,.y_cord_width_p(y_cord_width_p)
                             ,.data_width_p (data_width_p )
                             ,.addr_width_p (addr_width_p )
                             ) pkt_encode
     (.clk_i(clk_i)

      // the memory request, from the core's data memory port
      ,.v_i    (core_mem_v    [1])
      ,.data_i (core_mem_wdata[1])
      ,.addr_i (core_mem_addr [1])
      ,.we_i   (core_mem_w    [1])
      ,.mask_i (core_mem_mask [1])
      ,.from_y_cord_i(my_y_i)
      ,.from_x_cord_i(my_x_i)
      ,.ret_store_cntr_o(ret_store_cntr)
      // directly out to the network!
      ,.v_o    (v_o   )
      ,.data_o (data_o)
      );

   // Return packet on successful remote store:
   // Todo: can we remove ready signal from return network?
		//when design is finished, load design into formal solver 
		//and check if ret_ready_i can be fase.
		//It's possible that the restrictions on the store network 
		//are sufficient to restrict traffic volume.
   // For now, we add blocking logic if ret_ready_i is 0.
   assign ret_v_o = remote_store_yumi && (my_x_i != from_x_cord || my_y_i != from_y_cord); //don't create a return packet if from cord = to cord (e.g. during software load, where we duplicate from/to coords in packets). 

   assign ret_data_o = {5'b0,from_y_cord,from_x_cord}; //format packet, all empty except from addr.
   
	  
   // synopsys translate off

   `declare_bsg_manycore_packet_s(addr_width_p, data_width_p, x_cord_width_p, y_cord_width_p);

   bsg_manycore_packet_s data_o_debug;
   assign data_o_debug = data_o;

   if (debug_p)
     always @(negedge clk_i)
       begin
          if (v_o & ready_o)
            $display("proc sending packet %x (op=%x, addr=%x, data=%x, y_cord=%x, x_cord=%x), bit_mask=%x, core_mem_wdata=%x, core_mem_addr=%x"
                     , data_o_debug
                     , data_o_debug.op
                     , data_o_debug.addr
                     , data_o_debug.data
                     , data_o_debug.y_cord
                     , data_o_debug.x_cord
                     , core_mem_mask [1]
                     , core_mem_wdata[1]
                     , core_mem_addr [1]
                     );
       end

   // synopsys translate on

   wire [data_width_p-1:0] unused_data;
   wire                    unused_valid;

   // we create dedicated signals for these wires to allow easy access for "bind" statements
   wire [2:0]              xbar_port_v_in = {
                                              // request to write only if we are not sending a remote store packet
                                              // we check the high bit only for performance
                                               core_mem_v[1] & ~core_mem_addr[1][31]
                                              , remote_store_v
                                              , core_mem_v[0]
                                              };

   // proc data port sometimes writes, the network port always writes, proc inst port never writes
   wire [2:0]                    xbar_port_we_in   = { core_mem_w[1], 1'b1, 1'b0};
   wire [2:0]                    xbar_port_yumi_out;
   wire [2:0] [data_width_p-1:0] xbar_port_data_in = { core_mem_wdata [1], remote_store_data, core_mem_wdata[0]};
   wire [2:0] [mem_width_lp-1:0] xbar_port_addr_in = {   core_mem_addr[1]  [2+:mem_width_lp]
                                                       , remote_store_addr [2+:mem_width_lp]
                                                       , core_mem_addr[0]  [2+:mem_width_lp]
                                                       };
   wire [2:0] [(data_width_p>>3)-1:0] xbar_port_mask_in = { core_mem_mask[1], remote_store_mask, core_mem_mask[0] };

   always @(negedge clk_i)
     if (0)
     begin
        if (~freeze_r)
          $display("x=%x y=%x xbar_v_i=%b xbar_w_i=%b xbar_port_yumi_out=%b xbar_addr_i[2,1,0]=%x,%x,%x, xbar_data_i[2,1,0]=%x,%x,%x, xbar_data_o[1,0]=%x,%x"
                   ,my_x_i
                   ,my_y_i
                   ,xbar_port_v_in
                   ,xbar_port_we_in
                   ,xbar_port_yumi_out
                   ,xbar_port_addr_in[2]*4,xbar_port_addr_in[1]*4,xbar_port_addr_in[0]*4
                   ,xbar_port_data_in[2], xbar_port_data_in[1], xbar_port_data_in[0]
                   ,core_mem_rdata[1], core_mem_rdata[0]
                   );
     end

   // the swizzle function changes how addresses are mapped to banks
   wire [2:0] [mem_width_lp-1:0] xbar_port_addr_in_swizzled;

   genvar                        i;

   for (i = 0; i < 3; i=i+1)
     begin: port
//      assign xbar_port_addr_in_swizzled[i] = { xbar_port_addr_in[i] };

        assign xbar_port_addr_in_swizzled[i] = { xbar_port_addr_in  [i][(mem_width_lp-1)-:1]   // top bit is inst/data
                                                 , xbar_port_addr_in[i][0]                 // and lowest bit determines bank
                                                 , xbar_port_addr_in[i][1]                 // and lowest bit determines bank						 
                                                 , xbar_port_addr_in[i][2+:(mem_width_lp-2)]
                                                 };

     end

   assign { core_mem_yumi[1], remote_store_yumi, core_mem_yumi[0] } = xbar_port_yumi_out;

   // potentially, we could get better bandwidth if we demultiplexed the remote store input port
   // into four two-element fifos, one per bank. then, the arb could arbitrate for
   // each bank using those fifos. this allows for reordering of remote_stores across
   // banks, eliminating head-of-line blocking on a bank conflict. however, this would eliminate our
   // guaranteed in-order delivery and violate sequential consistency; so it would require some
   // extra hw to enforce that; and tagging of memory fences inside packets.
   // we could most likely get rid of the cgni input fifo in this case.
   
  bsg_mem_banked_crossbar #
    (.num_ports_p  (3)
     ,.num_banks_p  (num_banks_p)
     ,.bank_size_p  (bank_size_p)
     ,.data_width_p (data_width_p)
//     ,.rr_lo_hi_p   (2'b10) // round robin
//     ,.rr_lo_hi_p   (2'b01) // deadlock
     ,.rr_lo_hi_p(0)          // local dmem has priority
     ,.debug_p(debug_p*4)  // mbt: debug, multiply addresses by 4.
//      ,.debug_p(4)
//     ,.debug_reads_p(0)
    ) banked_crossbar
    ( .clk_i   (clk_i)
     ,.reset_i (reset_i)
      ,.v_i    (xbar_port_v_in)

      ,.w_i     (xbar_port_we_in)
      ,.addr_i  (xbar_port_addr_in_swizzled)
      ,.data_i  (xbar_port_data_in)
      ,.mask_i  (xbar_port_mask_in)

      // whether the crossbar accepts the input
     ,.yumi_o  ( xbar_port_yumi_out                                     )
     ,.v_o     ({ core_mem_rv    [1], unused_valid, core_mem_rv    [0] })
     ,.data_o  ({ core_mem_rdata [1], unused_data,  core_mem_rdata [0] })
    );




endmodule
