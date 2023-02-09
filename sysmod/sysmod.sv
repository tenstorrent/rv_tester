module sysmod #(
    parameter int CLOCK_PERIOD_PS           =     500,
    parameter int SW_CLOCK_UPDATE_PERIOD_PS = 100_000,
    rv_tester_pkg::cfg_t CFG                =      '0,
    parameter int  NUM                      =      -1,
    `RV_TESTER_PARAMETERS(CFG)
)(
    input clk,
    input reset,
    input longint unsigned clocks,
    output bootstrap_t bootstrap,
    output rv_tester_pkg::interrupt_t interrupt
);
    import "DPI-C" function void sysmod_tick(dpic_pkg::c_handle sysmod_p, longint unsigned advance);
    import "DPI-C" context function void sysmod_flush_cbs(dpic_pkg::c_handle sysmod_p);
    import "DPI-C" function dpic_pkg::c_handle sysmod_get(int num);
    import "DPI-C" context function void sysmod_set_scope(dpic_pkg::c_handle sysmod_p);
    import "DPI-C" function void sysmod_reset(dpic_pkg::c_handle sysmod_p);

    dpic_pkg::c_handle _sm;
    bit sysmod_poll = '1;

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            _sm = sysmod_get(NUM);
            sysmod_set_scope(_sm);
            sysmod_reset(_sm);
            sysmod_poll = cvm_plusargs::get_bool("sysmod_poll") != '0;
            /* verilator lint_on BLKSEQ */
        end
    end

    assign bootstrap.boot_addr = 1 << 31;

    logic terminated = '0;
    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKANDNBLK */
            terminated <= '0;
            /* verilator lint_on BLKANDNBLK */
        end
    end

    function automatic void sysmod_terminate (byte unsigned call_finish);
        // exit gracefully
        if (call_finish != 0) begin
            $finish();
        end
        terminated = '1;
    endfunction
    export "DPI-C" function sysmod_terminate;

    typedef longint unsigned LU;
    localparam longint unsigned TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(CLOCK_PERIOD_PS);
    always @(posedge clk) begin
        if (0 == (clocks % TICKS)) begin
            if (_sm != dpic_pkg::nil) begin
              sysmod_tick(_sm, TICKS);
            end
        end
    end

    rv_tester_pkg::interrupt_t /* verilator lint_off BLKANDNBLK */ interrupt_d /* verilator lint_on BLKANDNBLK */;
    rv_tester_pkg::interrupt_t interrupt_q;
    assign interrupt = interrupt_q;

    function automatic sysmod_timer_interrupt (unsigned hartid, unsigned val);
      $display("[SYSMOD] mti = %d", val);
      interrupt_d.mti = val;
    endfunction
    export "DPI-C" function sysmod_timer_interrupt;

    function automatic sysmod_sw_interrupt (unsigned hartid, unsigned val);
      $display("[SYSMOD] msi = %d", val);
      interrupt_d.msi = val;
    endfunction
    export "DPI-C" function sysmod_sw_interrupt;

    always @(posedge clk) begin
        interrupt_q <= interrupt_d;
        if (reset) begin
            interrupt_d <= '0;
        end
        if (_sm != dpic_pkg::nil && sysmod_poll) begin
          sysmod_flush_cbs(_sm);
        end
    end

endmodule
