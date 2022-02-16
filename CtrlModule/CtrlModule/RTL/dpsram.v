module dpsram#(parameter RAM_SIZE = 32768, parameter ADDRESS_WIDTH = 15, parameter WORD_SIZE = 8)(
    output wire [WORD_SIZE-1:0] q1,
    output wire [WORD_SIZE-1:0] q2,
    input wire [ADDRESS_WIDTH-1:0] a1,
    input wire [ADDRESS_WIDTH-1:0] a2,
    input wire [WORD_SIZE-1:0] d1,
    input wire [WORD_SIZE-1:0] d2,
    input wire clk,
    input wire wren1,
    input wire wren2);

   reg[WORD_SIZE-1:0] mem [0:RAM_SIZE-1] /* synthesis ramstyle = "M144K" */;
   reg[ADDRESS_WIDTH-1:0] address_latched1;
   reg[ADDRESS_WIDTH-1:0] address_latched2;

   always @(posedge clk) begin
     if (wren1)
         mem[a1] <= d1;
     if (wren2)
         mem[a2] <= d2;
      address_latched1 <= a1;
      address_latched2 <= a2;
   end

   assign q1 = mem[address_latched1];
   assign q2 = mem[address_latched2];
endmodule
