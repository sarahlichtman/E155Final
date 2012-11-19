// vga.sv
// 12 November 2012 Stephen Pinto
// vgaController code provided on 20 October 2011 by Karl_Wang & David_Harris@hmc.edu
// VGA driver with image generator (from bitmap file)

module final_vga(input  logic       clk, // sclk, // System clock & SPI clock from PIC
          input logic [3:0]   s,
          // input logic       note_sdi,              // SPI Data for notes being played
          output logic       vgaclk,            // 25 MHz VGA clock
          output logic       hsync, vsync, sync_b,  // to monitor & DAC
          output logic [7:0] r, g, b);          // to video DAC

  logic [9:0] x, y;
  logic [7:0] r_int, g_int, b_int;
  // logic [7:0] notes;

  // spi_slave_receive_only note_spi(sclk,note_sdi,notes);

  pll pll_inst (
  .areset ( areset_sig ),
  .inclk0 ( inclk0_sig ),
  .c0 ( c0_sig ),
  .locked ( locked_sig )
  );

  // Use a PLL to create the 25.175 MHz VGA pixel clock
  // 25.175 Mhz clk period = 39.772 ns
  // Screen is 800 clocks wide by 525 tall, but only 640 x 480 used for display
  // H  Sync = 1/(39.772 ns * 800) = 31.470 KHz
  // Vsync = 31.474 KHz / 525 = 59.94 Hz (~60 Hz refresh rate)
  pll vgapll(.inclk0(clk),  .c0(vgaclk));

  // user-defined module to determine pixel color
  videoGen videoGen(x, y,/* notes,*/ s, r_int, g_int, b_int);

  // generate monitor timing signals
  vgaController vgaCont(vgaclk, hsync, vsync, sync_b,
                        r_int, g_int, b_int, r, g, b, x, y);
endmodule

module vgaController #(parameter HMAX   = 10'd800,
                                 VMAX   = 10'd525,
                      HSTART = 10'd152,
                      WIDTH  = 10'd640,
                      VSTART = 10'd37,
                      HEIGHT = 10'd480)
              (input  logic       vgaclk,
              output logic       hsync, vsync, sync_b,
              input  logic [7:0] r_int, g_int, b_int,
              output logic [7:0] r, g, b,
              output logic [9:0] x, y);

  logic [9:0] hcnt, vcnt;
  logic       oldhsync;
  logic       valid;

  // counters for horizontal and vertical positions
  always @(posedge vgaclk) begin
    if (hcnt >= HMAX) hcnt = 0;
    else hcnt++;
   if (hsync & ~oldhsync) begin // start of hsync; advance to next row
     if (vcnt >= VMAX) vcnt = 0;
      else vcnt++;
    end
    oldhsync = hsync;
  end

  // compute sync signals (active low)
  assign hsync = ~(hcnt >= 10'd8 & hcnt < 10'd104); // horizontal sync
  assign vsync = ~(vcnt >= 2 & vcnt < 4); // vertical sync
  assign sync_b = hsync | vsync;

  // determine x and y positions
  assign x = hcnt - HSTART;
  assign y = vcnt - VSTART;

  // force outputs to black when outside the legal display area
  assign valid = (hcnt >= HSTART & hcnt < HSTART+WIDTH &
                  vcnt >= VSTART & vcnt < VSTART+HEIGHT);
  assign {r,g,b} = valid ? {r_int,g_int,b_int} : 24'b0;
endmodule

module videoGen(input  logic [9:0] x, y,
               input  logic [3:0] s,
               // input logic [7:0] notes,
               output logic [7:0] r_int, g_int, b_int);

  logic pixel;

  musicgenrom mgrom(x, y,/* notes,*/ s, pixel);

  assign {r_int, g_int, b_int} = pixel ? 24'h000000 : 24'hFFFFFF;

endmodule

module musicgenrom(input logic[9:0] x, y,
                   input logic [3:0] s,
                    // input logic[7:0] notes,
                    output logic pixel);
  logic [639:0] mem[255:0];
  logic [639:0] line;
  logic note_on;
  logic on_staffline;

  // integer k;
  initial begin
    $readmemb("scale.pbm",mem);
    // $display("Contents of mem after reading data file:");
    // for (k=0; k<226; k=k+1) $display("%d:%h",k,mem[k]);
  end

  // We have a picture of a scale but we only want the notes that are being played to be displayed.
  // Each of the magic numbers below are where the pixels for each note start and stop in the image.
  assign note_on = x < 110 || (x >= 110 && x < 186 && s[3]) || (x >= 186 && x < 245 && s[2]) ||
                    (x >= 245 && x < 311 && s[1]) || (x >= 311 && x < 377 && s[0]) || x >= 377;
                    // (x >= 377 && x < 444 && notes[3]) || (x >= 444 && x < 510 && notes[2]) ||
                    // (x >= 510 && x < 576 && notes[1]) || (x >= 576 && notes[0]);
  assign on_staffline = (y>=171 && y<=175) || (y>=203 && y<=207) || (y>=233 && y<=237) || (y>=265 && y<=269) || (y>=295 && y<=299);
  // Center image on screen. It's 226 pixels tall, so it should have 127 pixels of buffer above and below it.
  assign line = (y > 127 && y < 354) ? mem[y - 8'd128] : 640'd0;
  //assign line = mem[y];
  // Since the image is a bit map, the pixel is simply 1 or 0 for black or white.
  assign pixel = on_staffline ? 1 : (note_on ? line[10'd639-x] : 0);

endmodule
//
//// If the slave only need to received data from the master
//// Slave reduces to a simple shift register given by following HDL:
//module spi_slave_receive_only(input logic sck, //from master
//          input logic sdi, //from master
//          output logic[7:0] q); // data received
//
//  always_ff @(posedge sck)
//    q <={q[6:0], sdi}; //shift register
//
//endmodule

