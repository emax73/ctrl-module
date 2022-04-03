module bootloader#(parameter CONFIG_ON_STARTUP = 1, parameter ROM_LOCATION = 19'h30000, parameter ROM_END = 16'he000)(
	input wire clk,
	input wire[31:0] host_bootdata,
	input wire host_bootdata_req,
	output reg host_bootdata_ack,
	input wire host_reset,
	output reg[7:0] romwrite_data,
	output reg romwrite_wr,
	output reg[18:0] romwrite_addr,
	output reg rom_initialised);
	
	localparam ROMW_IDLE = 0;
	localparam ROMW_WRITE = 1;
	localparam ROMW_INC = 2;
	localparam ROMW_NEXT = 3;
	localparam ROMW_WAIT = 4;
	localparam ROMW_END = 5;
	localparam ROMW_PARKED = 6;
	localparam ROMW_WAITRESET = 7;

	reg[2:0] romwrite_status = CONFIG_ON_STARTUP == 1 ? ROMW_IDLE : ROMW_END;
	reg[1:0] romwrite_bytecount = 0;
	reg[31:0] romwrite_datareg;
	reg romwrite_reset_a = 1'b0;
	reg romwrite_reset_b = 1'b0;

	always @(posedge clk) begin
		case (romwrite_status)
			ROMW_IDLE: begin
				romwrite_status <= ROMW_WAIT;
				romwrite_addr[18:0] <= ROM_LOCATION;
				rom_initialised <= 1'b0;
				romwrite_wr <= 1'b0;
				romwrite_bytecount[1:0] <= 2'b00;
				host_bootdata_ack <= 1'b0;
			end
			ROMW_WAIT: if (host_bootdata_req) begin
				romwrite_status <= ROMW_WRITE;
				romwrite_datareg[31:0] <= host_bootdata[31:0];
			end else if (host_reset == 1'b1) romwrite_status <= ROMW_WAITRESET;
			ROMW_WRITE: begin
				romwrite_data[7:0] <= romwrite_datareg[31:24];
				romwrite_wr <= 1'b1;
				romwrite_status <= ROMW_INC;
				if (host_reset == 1'b1) romwrite_status <= ROMW_WAITRESET;
			end
			ROMW_INC: begin
				romwrite_bytecount[1:0] <= romwrite_bytecount[1:0] + 2'd1;
				romwrite_addr[18:0] <= romwrite_addr[18:0] + 19'd1;
				romwrite_status <= (romwrite_bytecount[1:0] == 2'd3) ? ROMW_NEXT : ROMW_WRITE;
				romwrite_datareg[31:8] <= romwrite_datareg[23:0];
				romwrite_wr <= 1'b0;
				if (host_reset == 1'b1) romwrite_status <= ROMW_WAITRESET;
			end
			ROMW_NEXT: begin
				if (host_bootdata_ack == 1'b0) begin
					host_bootdata_ack <= 1'b1;
				end else if (host_bootdata_ack == 1'b1 && host_bootdata_req == 1'b0) begin
					host_bootdata_ack <= 1'b0;
					romwrite_status <= romwrite_addr[15:0] == ROM_END ? ROMW_END : ROMW_WAIT;
				end else if (host_reset == 1'b1) romwrite_status <= ROMW_WAITRESET;
			end
			ROMW_END: begin
				rom_initialised <= 1'b1;
				romwrite_status <= ROMW_PARKED;
// 				romwrite_reset_a <= !romwrite_reset_a;
			end
			ROMW_PARKED: if (host_reset == 1'b1) romwrite_status <= ROMW_WAITRESET;
			ROMW_WAITRESET: begin
				if (host_reset == 1'b0) romwrite_status <= ROMW_IDLE;
				romwrite_wr <= 1'b0;
			end
		endcase
	end

endmodule
