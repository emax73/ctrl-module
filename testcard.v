/* This file is part of fpga-spec by ZXMicroJack - see LICENSE.txt for moreinfo */
module testcard(
  input [10 : 0] x_cnt,
  input [9 : 0]  y_cnt,
  input hsync_de,
  input vsync_de,
  input vga_clk,
  input[63:0] dbg,
  output reg[11:0] bar_data
  );

  // 800 x 600
  parameter H_ActivePix=800;            //显示时序段（Display interval c）
  parameter Hde_start=270;
  parameter Hde_end=1270;
  parameter Vde_start=34;
  parameter Vde_end=784;
  parameter V_ActivePix=600;            //显示时序段（Display interval c）

  // 640 x 480
  // parameter H_ActivePix=640;
  // parameter V_ActivePix=480;
  // parameter Hde_start=144;
  // parameter Hde_end=784;
  // parameter Vde_start=35;
  // parameter Vde_end=515;

	parameter bar_interval = H_ActivePix/8;
	parameter bar_interval2 = H_ActivePix/12;
  parameter top = V_ActivePix / 2 + Vde_start;
  parameter bottom = V_ActivePix + Vde_start - 32;
  parameter bit = H_ActivePix / 64;

  wire[11:0] bitn;


  assign bitn = (x_cnt - Hde_start);

	always @(negedge vga_clk) begin
		if (y_cnt < top) begin
			if (x_cnt==Hde_start)
				bar_data<= 12'hf00;              //red
			else if (x_cnt==Hde_start + bar_interval)
				bar_data<= 12'h0f0;              //green
			else if (x_cnt==Hde_start + bar_interval*2)
				bar_data<=12'h00f;               //blue
			else if (x_cnt==Hde_start + bar_interval*3)
				bar_data<=12'hf0f;               //purple
			else if (x_cnt==Hde_start + bar_interval*4)
				bar_data<=12'hff0;               //yellow
			else if (x_cnt==Hde_start + bar_interval*5)
				bar_data<=12'h0ff;               //cyan
			else if (x_cnt==Hde_start + bar_interval*6)
				bar_data<=12'hfff;               //white
			else if (x_cnt==Hde_start + bar_interval*7)
				bar_data<=12'hf80;               //orange
			else if (x_cnt==Hde_start + bar_interval*8)
				bar_data<=12'h000;               //black
		end else if (y_cnt < bottom) begin
			if (x_cnt==Hde_start)
				bar_data<= 12'h800;              //red
			else if (x_cnt==Hde_start + bar_interval2)
				bar_data<= 12'h400;              //green
			else if (x_cnt==Hde_start + bar_interval2*2)
				bar_data<=12'h200;               //blue
			else if (x_cnt==Hde_start + bar_interval2*3)
				bar_data<=12'h100;               //purple
			else if (x_cnt==Hde_start + bar_interval2*4)
				bar_data<=12'h080;               //yellow
			else if (x_cnt==Hde_start + bar_interval2*5)
				bar_data<=12'h040;               //cyan
			else if (x_cnt==Hde_start + bar_interval2*6)
				bar_data<=12'h020;               //white
			else if (x_cnt==Hde_start + bar_interval2*7)
				bar_data<=12'h010;               //orange
			else if (x_cnt==Hde_start + bar_interval2*8)
				bar_data<=12'h008;               //black
			else if (x_cnt==Hde_start + bar_interval2*9)
				bar_data<=12'h004;               //black
			else if (x_cnt==Hde_start + bar_interval2*10)
				bar_data<=12'h002;               //black
			else if (x_cnt==Hde_start + bar_interval2*11)
				bar_data<=12'h001;               //black
		end else begin
      bar_data <= dbg[bitn[11:6]] ? 12'h777 : 12'h000;
    end
	end

endmodule
