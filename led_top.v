`timescale 1ns / 1ps
module led_top(
    input sys_clk_i,
    input sys_rst_i,

    output led_1,
		output led_2,

		output vga_hs,
		output vga_vs,
		output[3:0] vga_r,
		output[3:0] vga_g,
		output[3:0] vga_b,
		input kbd_data,
		input kbd_clk,
    // input mse_data,
		// input mse_clk,
    output aud_left,
    output aud_right,
    output SD_cs,
    output SD_clk,
    output SD_datain,
    input SD_dataout,
    // input uart_rx,
    // output uart_tx
    output [18:0] SRAM_ADDR,
    output SRAM_WE_N,
    inout [7:0] SRAM_DQ,
    output[3:0] tp,
		input juart_rx,
		output juart_tx
    );

    wire[15:0] dswitch;

		assign SRAM_ADDR = 19'd0;
		assign SRAM_WE_N = 1'b1;
		assign SRAM_DQ = 8'hZZ;
	
  //------->8------->8------->8------->8------->8------->8------->8------->8------->8
  // test pads to connect to logic analyser
	assign tp[3:0] = 4'h0;

  assign aud_left = ear_in;
  assign aud_right = ear_in;

  wire[3:0] vga_r_o, vga_b_o, vga_g_o;
  wire[63:0] dbg2;
  display display(
    .clk50m(sys_clk_i),
    .rstn(sys_rst_i),

    // outputs
    .vga_hs(vga_hs),
    .vga_vs(vga_vs),
    .vga_r(vga_r_o),
    .vga_g(vga_g_o),
    .vga_b(vga_b_o),
    .dbg(dbg2)
    );

  reg host_divert_keyboard;

  //------->8------->8------->8------->8------->8------->8------->8------->8------->8
  wire ear_in;
  reg[6:0] clkdiv;
  always @(posedge sys_clk_i) clkdiv <= clkdiv + 1'b1;

  assign clk25 = clkdiv[0];
  assign clk12m5 = clkdiv[1];
  assign clk6m25 = clkdiv[2];
  assign clk3m125 = clkdiv[3];
  assign clk1m5625 = clkdiv[4];
  assign clk781k25 = clkdiv[5];
  assign clk390k625 = clkdiv[6];

  //------->8------->8------->8------->8------->8------->8------->8------->8------->8
  // use sys_clk for 800x600 and clk25 for 640x480

  wire[15:0] size;
  wire[31:0] host_bootdata;
	wire[7:0] host_bootdata_type;
	wire host_bootdata_sig;
  wire host_bootdata_ack;

  CtrlModule MyCtrlModule (
		.clk(clk6m25),
    .clk26(sys_clk_i),
		.reset_n(sys_rst_i),

		//-- Video signals for OSD
		.vga_hsync(vga_hs),
		.vga_vsync(vga_vs),
		.osd_window(osd_window),
		.osd_pixel(osd_pixel),

		//-- PS2 keyboard
		.ps2k_clk_in(kbd_clk),
		.ps2k_dat_in(kbd_data),

		//-- SD card signals
		.spi_clk(SD_clk),
		.spi_mosi(SD_datain),
		.spi_miso(SD_dataout),
		.spi_cs(SD_cs),

		//-- DIP switches
		.dipswitches(dswitch),

		//-- Control signals
		.host_divert_keyboard(host_divert_keyboard),
		.host_divert_sdcard(host_divert_sdcard),

		// tape interface
    .ear_in(ear_in),
    .ear_out(ear_in),
    .clk390k625(clk390k625),
    .juart_rx(juart_rx),
    .juart_tx(juart_tx)
	);

  wire[7:0] vga_red_i, vga_green_i, vga_blue_i;
  assign vga_red_i = {vga_r_o[3:0], 4'h0};
  assign vga_green_i = {vga_g_o[3:0], 4'h0};
  assign vga_blue_i = {vga_b_o[3:0], 4'h0};

  wire[7:0] vga_red_o, vga_green_o, vga_blue_o;
  assign vga_r = vga_red_o[7:4];
  assign vga_g = vga_green_o[7:4];
  assign vga_b = vga_blue_o[7:4];

  // OSD Overlay
  OSD_Overlay overlay (
		// .clk(clk25),
    .clk(sys_clk_50m),
		.red_in(vga_red_i),
		.green_in(vga_green_i),
		.blue_in(vga_blue_i),
		.window_in(1'b1),
		.osd_window_in(osd_window),
		.osd_pixel_in(osd_pixel),
		.hsync_in(vga_hsync_i),
		.red_out(vga_red_o),
		.green_out(vga_green_o),
		.blue_out(vga_blue_o),
		.window_out( ),
		.scanline_ena(1'b0) //scandblr_reg[1])
	);
endmodule
