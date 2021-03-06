`include "bsg_manycore_packet.vh"

module bsg_manycore_pkt_decode
  #(
    x_cord_width_p   = "inv"
    , y_cord_width_p = "inv"
    , data_width_p   = "inv"
    , addr_width_p   = "inv"
    , packet_width_lp = `bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p)
    )
   (
    input   v_i
    ,input [packet_width_lp-1:0] data_i

    ,output logic pkt_freeze_o
    ,output logic pkt_unfreeze_o
    ,output logic pkt_unknown_o
    ,output logic lock_req_o

    ,output logic pkt_remote_store_o
    ,output logic [data_width_p-1:0] data_o
    ,output logic [addr_width_p-1:0] addr_o
    ,output logic [y_cord_width_p-1:0] from_y_cord_o
    ,output logic [x_cord_width_p-1:0] from_x_cord_o
    ,output logic [(data_width_p>>3)-1:0] mask_o
    ); 

   `declare_bsg_manycore_packet_s(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p);

   bsg_manycore_packet_s pkt;

   assign pkt = data_i;
   assign data_o = pkt.data;
   assign addr_o = pkt.addr;
   assign from_y_cord_o = pkt.from_y_cord;
   assign from_x_cord_o = pkt.from_x_cord;

   always_comb
     begin
        pkt_freeze_o        = 1'b0;
        pkt_unfreeze_o      = 1'b0;
        pkt_remote_store_o  = 1'b0;
        pkt_unknown_o       = 1'b0;
        lock_req_o          = 1'b0;
        mask_o              = 0;

        if (v_i)
          begin
             case (pkt.op)
               1:
                 begin
                    pkt_remote_store_o = 1'b1;
                    mask_o             = pkt.op_ex;
                 end
               2:
                 if (~|pkt.addr[addr_width_p-1:0])
                   begin
                      pkt_freeze_o   = pkt.data[0];
                      pkt_unfreeze_o = ~pkt.data[0];
                   end
                 else
                   pkt_unknown_o = 1'b1;
               3: 
                 begin
                   lock_req_o = 1'b1;
                 end
               default:
                 pkt_unknown_o = 1'b1;
             endcase
          end
     end

endmodule
 
