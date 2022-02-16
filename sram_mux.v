module sram_mux(
	output reg[18:0] addr,
	inout[7:0] data,
	output n_wr,
	input clk50m,

	input[18:0] addr1,
	input[7:0] data_in1,
	output reg[7:0] data_out1,
	input rd1,
	input wr1,
	output ack,

	input[18:0] addr2,
	input[7:0] data_in2,
	output reg[7:0] data_out2,
	input n_wr2,

	output db
	);

	reg[7:0] data_in;
	wire[7:0] data_out;

	reg write = 1'b0;
	assign data[7:0] = write ? data_in[7:0] : 8'bZZZZZZZZ;
	assign data_out[7:0] = write ? 8'h00 : data[7:0];
	assign n_wr = !write;

	reg[1:0] busy1 = 2'b0;
	reg[27:0] sr2prev = 28'h1fffffff;
	reg if2evt = 1'b0;

	reg if_active = 1'b0;

	reg rd1prev = 1'b0, wr1prev = 1'b0;
	reg if1evt = 1'b0;
  reg ready = 1'b1;

	assign db = ack;
  assign ack = ready;

	always @(posedge clk50m) begin
		// detect sram interface 1
		{rd1prev, wr1prev} <= { rd1, wr1 };
		if ((!rd1prev && rd1) || (!wr1prev && wr1))
			{ready, if1evt} <= 2'b01;

			// detect sram interface 2
		sr2prev <= {addr2[18:0], data_in2[7:0], n_wr2};
		if (sr2prev != {addr2[18:0], data_in2[7:0], n_wr2})
			if2evt <= 1'b1;

		// is read in operation
		if (busy1[1:0] != 0) begin
			busy1[1:0] <= busy1[1:0] - 1;
			// has allowed enough time for sram to respond
			if (busy1[1:0] == 2'b01) begin
				// if 1- reset write flag, read
				if (if_active) begin
					write <= 1'b0;
					if (!n_wr2) data_out2[7:0] <= data_out[7:0];

				// if 0- reset write flag, read
				end else begin
					ready <= 1'b1; // ack
					write <= 1'b0; // reset write
					if (rd1) data_out1[7:0] <= data_out[7:0]; // update read data
				end
			end
		// ram is free and event 2 has happened
		end else if (busy1[1:0] == 0 && if2evt) begin
			busy1[1:0] <= 2'b11; // reload delay
			if2evt <= 1'b0; // reset event
			write <= !n_wr2; // set write flag
			data_in[7:0] <= data_in2[7:0]; // write data in in either case
			addr[18:0] <= addr2[18:0];
			if_active <= 1'b1; // if 1 active

		end else if (busy1[1:0] == 0 && if1evt) begin
			busy1[1:0] <= 2'b11; // reload delay
			if1evt <= 1'b0; // reset event
			write <= wr1; // set write flag
			data_in[7:0] <= data_in1[7:0]; // write data in in either case
			addr[18:0] <= addr1[18:0];
			if_active <= 1'b0; // if 0 active
		end
	end
endmodule
