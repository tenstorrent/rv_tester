// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

module interrupts
  import rv_tester_params::*;
  #(
    parameter int NUM = -1,
    `TOPOLOGY,
    `RV_TESTER_TRANSACTIONS_INTERRUPTS_OUTPUT_PARAMS
    )
  (
   input logic clk,
   input logic sys_reset,
   input logic reset,
   input logic boot_done,
   input logic [63:0] clocks,
   /* verilator lint_off BLKANDNBLK */
   output logic nmi,
   output logic mti,
   /* verilator lint_on BLKANDNBLK */
   `RV_TESTER_TRANSACTIONS_INTERRUPTS_OUTPUT_PORTS
   );

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.INTERRUPTS.ID, NUM);

  `CVM_REGISTRY_SET_SCOPE(location)

  always @(posedge clk) begin
    if (sys_reset) begin
      nmi <= '0;
      mti <= '0;
    end
  end

  // -------------------------
  // NMI Logic
  // -------------------------
  logic nmi_asserted;
  logic nmi_d1;
  always @(posedge clk) begin
    nmi_d1 <= nmi;
    if (reset) begin
      nmi_asserted <= '0;
    end else begin
      if (nmi & ~nmi_d1)
        nmi_asserted <= '1;
      if (nmi_d1 & ~nmi)
        nmi_asserted <= '0;
    end
  end

  // -------------------------
  // MTI Logic
  // -------------------------
  logic mti_asserted;
  logic mti_d1;
  always @(posedge clk) begin
    mti_d1 <= mti;
    if (reset) begin
      mti_asserted <= '0;
    end else begin
      if (mti & ~mti_d1)
        mti_asserted <= '1;
      if (mti_d1 & ~mti)
        mti_asserted <= '0;
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  // m_nmi_assert_tick
  logic nmi_assert_tick;
  rv_tester_tick_generator #(.NAME("nmi")) nmi_assert_tick_generator (.clk(clk), .reset(reset || !boot_done), .inhibit(nmi_asserted), .tick(nmi_assert_tick), .last());
  assign m_nmi_assert_ticks[0].valid = nmi_assert_tick & (location != cvm_topology::nil);
  assign m_nmi_assert_ticks[0].data.location = location;

  // m_mti_assert_tick
  logic mti_assert_tick;
  rv_tester_tick_generator #(.NAME("mti")) mti_assert_tick_generator (.clk(clk), .reset(reset || !boot_done), .inhibit(mti_asserted), .tick(mti_assert_tick), .last());
  assign m_mti_assert_ticks[0].valid = mti_assert_tick & (location != cvm_topology::nil);
  assign m_mti_assert_ticks[0].data.location = location;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------

  export "DPI-C" function drive_nmi;
  export "DPI-C" function drive_mti;

  function void drive_nmi(bit val);
    nmi = val;
  endfunction

  function void drive_mti(bit val);
    mti = val;
  endfunction

endmodule
