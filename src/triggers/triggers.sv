module triggers
  import rv_tester_params::*;
  #(
    parameter int NUM = -1,
    `TOPOLOGY,
    `RV_TESTER_TRANSACTIONS_TRIGGERS_OUTPUT_PARAMS
    )
  (
   input logic tb_clk,
   input logic tb_reset,
   input logic clk,
   input logic reset,
   input event_trigger_intf_t event_trigger_vec [NHARTS-1:0],
   `RV_TESTER_TRANSACTIONS_TRIGGERS_OUTPUT_PORTS
   );

  import "DPI-C" function int unsigned get_random_in_range(int unsigned min, int unsigned max);

  logic [TRIGGER_COUNT-1:0] hart_specific_event_trigger;
  logic [NHARTS-1:0] event_trigger_vlds;
  logic [NHARTS-1:0] event_trigger_vlds_latched;
  
  genvar i;
  generate
    for (i = 0; i < TRIGGER_COUNT; i = i + 1) begin
      assign hart_specific_event_trigger[i] = event_trigger_vec[NUM][i].valid;
    end
  endgenerate

  always_comb begin
    for (int hart_num = 0; hart_num < NHARTS; hart_num++) begin
      event_trigger_vlds[hart_num] = 0;
      for (int trigger_num = 0; trigger_num < TRIGGER_COUNT; trigger_num++) begin
        event_trigger_vlds[hart_num] |= event_trigger_vec[hart_num][trigger_num].valid;
      end
    end
  end
  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.TRIGGERS.ID, NUM);

  `CVM_REGISTRY_SET_SCOPE(location)

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------
  logic reset_d1;
  int unsigned tb_clocks = 0;
  int unsigned interrupt_injection_count;
  int unsigned interrupt_injection_initial_delay;
  int unsigned interrupt_injection_rand_delay_min;
  int unsigned interrupt_injection_rand_delay_max;
  always @(posedge tb_clk) begin
    tb_clocks <= tb_clocks + 1;
    reset_d1 <= reset;
    if (~reset & reset_d1) begin
      interrupt_injection_count <= cvm_plusargs::get_int("interrupt_injection_count");
      interrupt_injection_initial_delay <= cvm_rand::get("interrupt_injection_initial_delay");
      interrupt_injection_rand_delay_min <= cvm_plusargs::get_int("interrupt_injection_rand_delay_min");
      interrupt_injection_rand_delay_max <= cvm_plusargs::get_int("interrupt_injection_rand_delay_max");
    end
  end

  bit trigger_interrupt = 0;
  bit trigger_interrupt_delayed = 0;
  int unsigned clocks = 0;
  int unsigned cur_interrupt_count = 0;
  int unsigned next_interrupt_clock = 0;
  bit interrupt_sequence_active = 0;

  /* verilator lint_off BLKANDNBLK */
  /* verilator lint_off BLKSEQ */
  always @(posedge clk) begin
    clocks <= clocks + 1;
    trigger_interrupt_delayed <= trigger_interrupt;
    if (hart_specific_event_trigger[INTERRUPT]) begin
      cur_interrupt_count = interrupt_injection_count;
      next_interrupt_clock = clocks + interrupt_injection_initial_delay;
      // Latch event_trigger_vlds when initial trigger occurs
      event_trigger_vlds_latched <= event_trigger_vlds;
      interrupt_sequence_active = (cur_interrupt_count > 0);
    end
    if (reset) begin
      trigger_interrupt <= 1'b0;
      event_trigger_vlds_latched <= '0;
      interrupt_sequence_active <= 1'b0;
    end
    else begin
      trigger_interrupt = 1'b0;
      if ((cur_interrupt_count > 0) && (clocks == next_interrupt_clock)) begin
        trigger_interrupt = 1'b1;
        cur_interrupt_count <= cur_interrupt_count - 1;
        next_interrupt_clock <= clocks + get_random_in_range(interrupt_injection_rand_delay_min, interrupt_injection_rand_delay_max);
      end
      // Clear latched value and flag when all interrupts are sent
      if (cur_interrupt_count == 0) begin
        event_trigger_vlds_latched <= '0;
        interrupt_sequence_active <= 1'b0;
      end
    end
  end
  /* verilator lint_on BLKSEQ */
  /* verilator lint_on BLKANDNBLK */

  // m_event_trigger_ticks
  assign m_event_trigger_ticks[0].valid = trigger_interrupt & (location != cvm_topology::nil);
  assign m_event_trigger_ticks[0].data.location = location;
  // Use latched value when interrupt sequence is active, otherwise use current value
  assign m_event_trigger_ticks[0].data.per_core_evt_vector = {{(32 - NHARTS){1'b0}}, 
                                                              interrupt_sequence_active ? event_trigger_vlds_latched : event_trigger_vlds};
  /* verilator lint_off WIDTHEXPAND */
  assign m_event_trigger_ticks[0].data.event_trigger = hart_specific_event_trigger;
  /* verilator lint_on WIDTHEXPAND */
  assign m_event_trigger_delayed_ticks[0].valid = trigger_interrupt_delayed & (location != cvm_topology::nil);
  assign m_event_trigger_delayed_ticks[0].data.location = location;

endmodule
