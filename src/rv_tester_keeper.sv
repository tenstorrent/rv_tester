module dpi_rv_tester_keeper_ctrl (input bit clk, output bit load, input bit data  );

  bit send;

  // Export DPI function to control load and send signals
  function void dpi_rv_tester_keeper_drive_control(
                                                   input bit load_i,
                                                   input bit send_i
                                                   );
    load = load_i;
    send = send_i;
  endfunction
  export "DPI-C" function dpi_rv_tester_keeper_drive_control;

  import "DPI-C" function void dpi_rv_tester_keeper_send_data(input bit data);
  always @(posedge clk) if (send==1'b1) dpi_rv_tester_keeper_send_data(data);

endmodule


