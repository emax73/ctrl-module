/* This file is part of fpga-spec by ZXMicroJack - see LICENSE.txt for moreinfo */
`include "settings.h"

module display(
  input rstn,
  input clk50m,
  input sys_clk,

  // vga connectors
  output vga_hs,
  output vga_vs,
  output [3:0] vga_r,
  output [3:0] vga_g,
  output [3:0] vga_b,

  input[63:0] dbg
  );

  wire [10 : 0] x_cnt;
  wire [9 : 0]  y_cnt;
  wire hsync_de;
  wire vsync_de;
  wire vga_clk;
`ifdef DEBUG
  wire[11:0] text_output;
`endif
  wire[11:0] rgb_out;
	wire[11:0] bar_data;
//	parameter bar_interval = 640/8;
//	parameter bar_interval2 = 640/12;

  assign rgb_out = bar_data;

	testcard testcard(
		.x_cnt(x_cnt),
		.y_cnt(y_cnt),
		.hsync_de(hsync_de),
		.vsync_de(vsync_de),
		.vga_clk(vga_clk),
		.dbg(dbg),
		.bar_data(bar_data)
	);

  vga vga(
  			.clk50m(clk50m),
  			.rstn(rstn),
  			.vga_hs(vga_hs),
  			.vga_vs(vga_vs),
  			.vga_r(vga_r),
  			.vga_g(vga_g),
  			.vga_b(vga_b),
        .x_cnt(x_cnt),
        .y_cnt(y_cnt),
        .hsync_de(hsync_de),
        .vsync_de(vsync_de),
        .rgb_out(rgb_out),
        .vga_clk(vga_clk)
  );

endmodule
