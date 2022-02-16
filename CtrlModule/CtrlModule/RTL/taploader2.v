/* This file is part of fpga-spec by ZXMicroJack - see LICENSE.txt for moreinfo */
module tapsaver2#(
  parameter LEADER = 244,
  parameter SYNC = 73,
  parameter ONE = 195,
  parameter ZERO = 98
  )(
  output reg[7:0] data_out,
  output wire data_valid,
  output wire data_end,
  input wire ack,
  input wire ear,
  input wire clk,
  input wire clk50m
  );

  reg prev_ear = 1'b0;
  reg prev_ack = 1'b0;
  reg[9:0] counter = 1'b0;
  reg[3:0] nbits = 1'b0;
  reg[7:0] data = 1'b0;

  reg data_valid_a = 1'b0;
  reg data_valid_b = 1'b0;
  reg data_end_a = 1'b0;
  reg data_end_b = 1'b0;

  localparam NOTHING_TH = LEADER_TH + 50;
  localparam LEADER_TH = LEADER - 20;
  localparam ONE_TH = ONE - 20;
  localparam ZERO_TH = ZERO - 20;
  localparam SYNC_TH = SYNC - 20;

  localparam IDLE = 0;
  localparam IN_LEADER = 1;
  localparam IN_SYNC = 2;
  localparam IN_DATA = 3;

  reg[3:0] state = IDLE;

  assign data_valid = data_valid_a ^ data_valid_b;
  assign data_end = data_end_a ^ data_end_b;

  always @(posedge clk50m) begin
    prev_ack <= ack;

    if (prev_ack ^ ack) begin
      data_valid_a <= data_valid_b;
      data_end_a <= data_end_b;
    end

  end

  always @(posedge clk) begin
    prev_ear <= ear;
    counter <= counter + 1;

    // state of ear input
    if (prev_ear != ear) begin
      counter <= 1'b0;

      // check edges large to small
      if (counter > NOTHING_TH) begin
        state <= IDLE;

      end else if (counter > LEADER_TH) begin
        state <= IN_LEADER;

      end else if (counter > ONE_TH) begin
        if (state == IN_SYNC) state <= IN_DATA;
        else if (state == IN_DATA) begin
          data[7:0] <= {data[6:0], 1'b1};
          state <= IN_SYNC;
          nbits <= nbits + 1;
        end

      end else if (counter > ZERO_TH) begin
        if (state == IN_SYNC) state <= IN_DATA;
        else if (state == IN_DATA) begin
          data[7:0] <= {data[6:0], 1'b0};
          state <= IN_SYNC;
          nbits <= nbits + 1;
        end

      end else if (counter > SYNC_TH) begin
        if (state == IN_LEADER) begin
          state <= IN_SYNC;
          nbits <= 1'b0;
        end else if (state != IN_SYNC) state <= IDLE;
      end
    end // if (prev_ear != ear) begin

    if (nbits == 8) begin
      data_valid_b <= !data_valid_b;
      data_out <= data;
      nbits <= 1'b0;
    end

    if (state != IDLE && counter > NOTHING_TH) begin
      // data_valid_b <= data_valid_a;
      data_end_b <= !data_end_b;
      state <= IDLE;
    end

  end

endmodule

module taploader2#(
  parameter TURBO_1 = 49,
  parameter TURBO_0 = 24,
  parameter NORMAL_1 = 191,
  parameter NORMAL_0 = 95,
  parameter LEADER_PULSE = 242,
  parameter SYNC_PULSE = 75,
  parameter ONE_SECOND = 1627
  )(
  input wire[7:0] data_in,
  output reg data_req,
  input wire data_ready,
  output reg ack,
  output reg reset_out,
  input wire dend_in,
  input wire clk50m,
  input wire clk,
  input wire play,
  output reg ear_in,
  input wire turbo_loading //
  );

  reg[7:0] tap_pulse_count;
  reg[7:0] tap_pulse_reload;
  reg[12:0] tap_leader_count;
  reg[7:0] tap_data_byte;

  reg tap_output = 0;
  reg[2:0] tap_state = 3'b0;

  reg silence = 1'b0;
  reg[7:0] tap_data;
  reg tap_dend;

  parameter RESET = 7;
  parameter IDLE = 0;
  parameter NEW_BLOCK = 1;
  parameter NEW_BLOCK2 = 6;
  parameter LEADER = 2;
  parameter SYNC = 3;
  parameter DATA = 4;
  parameter PAUSE = 5;

  reg demand = 1'b0;
  reg prev_demand = 1'b0;
  reg reset = 1'b0;

  // read sdram on demand
  always @(posedge clk50m) begin
    if (reset) begin
      prev_demand <= demand;
    end else if (demand != prev_demand) begin
      data_req <= 1'b1;
      prev_demand <= demand;
    end else if (data_req && data_ready) begin
      data_req <= 1'b0;
      tap_data <= data_in;
      tap_dend <= dend_in;
      ack <= 1'b1;
    end else if (ack && !data_ready) begin
      ack <= 1'b0;
    end
  end

  always @(posedge clk) begin
    case (tap_state)
      RESET: begin
        reset <= 1'b1;
        reset_out <= 1'b1;
        tap_state <= IDLE;
      end

      IDLE: begin
        reset <= 1'b0;
        if (play) begin
          tap_state <= PAUSE;
          silence <= 1'b1;
          reset_out <= 1'b0;

          tap_pulse_reload <= LEADER_PULSE;
          tap_pulse_count <= LEADER_PULSE;
          tap_leader_count <= ONE_SECOND*1; // around 4 second of pause
        end
      end

      PAUSE:
        if (!play) tap_state <= RESET;
        else if (tap_leader_count == 0) begin
          tap_leader_count <= 0;
          tap_pulse_count <= 0;
          tap_leader_count <= 0;
          silence <= 1'b0;
          tap_state <= NEW_BLOCK;
        end

      NEW_BLOCK:
        if (!play) tap_state <= RESET;
        else begin
          tap_state <= NEW_BLOCK2;
        end

      NEW_BLOCK2:
        if (!play) tap_state <= RESET;
        else begin
          tap_state <= LEADER;
          tap_pulse_reload <= LEADER_PULSE;
          tap_pulse_count <= LEADER_PULSE;
          tap_leader_count <= ONE_SECOND*4; // around 4 seconds of leader
          demand <= ~demand;
        end

      LEADER:
        if (!play) tap_state <= RESET;
        else if (tap_leader_count == 0) begin
          tap_state <= SYNC;
          tap_pulse_reload <= SYNC_PULSE;
          tap_pulse_count <= SYNC_PULSE;
          tap_leader_count <= 2;
        end

      SYNC:
        if (!play) tap_state <= RESET;
        else if (tap_leader_count == 0) begin
          tap_data_byte[7:1] <= tap_data[6:0];
          demand <= ~demand;

          tap_state <= DATA;
          if (turbo_loading) begin
            tap_pulse_reload <= tap_data[7] ? TURBO_1 : TURBO_0;
            tap_pulse_count <= tap_data[7] ? TURBO_1 : TURBO_0;
          end else begin
            tap_pulse_reload <= tap_data[7] ? NORMAL_1 : NORMAL_0;
            tap_pulse_count <= tap_data[7] ? NORMAL_1 : NORMAL_0;
          end
          tap_leader_count <= 16;
        end

      DATA:
        if (!play) tap_state <= RESET;
        else if (tap_leader_count == 0) begin
          if (tap_dend) begin
            tap_state <= PAUSE;

            tap_pulse_reload <= LEADER_PULSE;
            tap_pulse_count <= LEADER_PULSE;
            tap_leader_count <= ONE_SECOND*1; // around 4 second of pause
            silence <= 1'b1;
            // demand <= ~demand;
          end else begin
            tap_data_byte[7:1] <= tap_data[6:0];
            // if (!tap_dend) demand <= ~demand;
            demand <= ~demand;

            // mechanism allows only half wave
            tap_leader_count <= 16;
            if (turbo_loading) begin
              tap_pulse_reload <= tap_data[7] ? TURBO_1 : TURBO_0;
              tap_pulse_count <= tap_data[7] ? TURBO_1 : TURBO_0;
            end else begin
              tap_pulse_reload <= tap_data[7] ? NORMAL_1 : NORMAL_0;
              tap_pulse_count <= tap_data[7] ? NORMAL_1 : NORMAL_0;
            end
          end
        end else if (tap_pulse_count == 0 && !tap_leader_count[0]) begin
          // reload next bit - every other pulse
          if (turbo_loading) begin
            tap_pulse_reload <= tap_data_byte[7] ? TURBO_1 : TURBO_0;
            tap_pulse_count <= tap_data_byte[7] ? TURBO_1 : TURBO_0;
          end else begin
            tap_pulse_reload <= tap_data_byte[7] ? NORMAL_1 : NORMAL_0;
            tap_pulse_count <= tap_data_byte[7] ? NORMAL_1 : NORMAL_0;
          end
          tap_data_byte[7:1] <= tap_data_byte[6:0];
        end
    endcase

    // oscillate the output at tap_pulse_reload clocks for
    // tap_leader_count number of oscillations.
    if (tap_leader_count) begin
      if (!tap_pulse_count) begin
        ear_in <= silence ? 1'b0 : !ear_in;
        tap_pulse_count <= tap_pulse_reload;
        tap_leader_count <= tap_leader_count - 1;
      end else tap_pulse_count <= tap_pulse_count - 1;
    end

    // CONSTANT LEADER SIGNAL
    // if (!tap_pulse_count) begin
    //   ear_in <= !ear_in;
    //   tap_pulse_count <= 30;
    // end else tap_pulse_count <= tap_pulse_count - 1;
  end

endmodule
