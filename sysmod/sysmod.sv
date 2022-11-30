module sysmod #(
    parameter int RESET_CLOCKS              =      10,
    parameter int CLOCK_PERIOD_PS           =     500,
    parameter int SW_CLOCK_UPDATE_PERIOD_PS = 100_000,
    rv_tester_pkg::cfg_t CFG                =      '0,
    parameter int  NUM                      =      -1,
    `RV_TESTER_PARAMETERS(CFG)
)(
    input clk,
    output reset,
    output bootstrap_t bootstrap,
    output rv_tester_pkg::interrupt_t interrupt
);
    import "DPI-C" function void sysmod_tick(chandle sysmod_p, longint unsigned advance);
    import "DPI-C" context function void sysmod_flush_cbs(chandle sysmod_p);

    import "DPI-C" function chandle sysmod_get(int num);

    import "DPI-C" context function void sysmod_set_scope(chandle sysmod_p);
    import "DPI-C" function void sysmod_compose(chandle sysmod_p, string memmap);
    import "DPI-C" function void sysmod_load_program(chandle sysmod_p, string prog);
    import "DPI-C" function void sysmod_reset(chandle sysmod_p);

    chandle _sm;
    initial begin 
        string prog;
        _sm = sysmod_get(NUM);
        $display("setting scope %m");
        sysmod_set_scope(_sm);
        sysmod_compose(_sm, "memmap.json");
        if ($value$plusargs("hex=%s", prog)) begin
          sysmod_load_program(_sm, prog);
        end
    end

    longint unsigned clocks = 0;
    always @(posedge clk) begin
        clocks <= clocks + 1;
    end

    typedef longint unsigned LU;

    assign reset = clocks < LU'(RESET_CLOCKS);
    assign bootstrap.boot_addr = 1 << 31;

    function automatic sysmod_terminate ();
        // exit gracefully
        $finish();
    endfunction
    export "DPI-C" function sysmod_terminate;

    localparam longint unsigned TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(CLOCK_PERIOD_PS);
    always @(posedge clk) begin
        // FIXME: should be queued up in separate thread
        if (0 == (clocks % TICKS)) begin
            sysmod_tick(_sm, TICKS);
        end
    end

    rv_tester_pkg::interrupt_t /* verilator lint_off BLKANDNBLK */ interrupt_d /* verilator lint_on BLKANDNBLK */;
    rv_tester_pkg::interrupt_t interrupt_q;
    assign interrupt = interrupt_q;

    function automatic sysmod_timer_interrupt (unsigned hartid, unsigned val);
      interrupt_d.timer = val;
    endfunction
    export "DPI-C" function sysmod_timer_interrupt;

    function automatic sysmod_sw_interrupt (unsigned hartid, unsigned val);
      interrupt_d.ipi = val;
    endfunction
    export "DPI-C" function sysmod_sw_interrupt;

    always_ff @(posedge clk) begin
        if (reset) interrupt_d <= '0;
        interrupt_q <= interrupt_d;

        // FIXME: add poll
        sysmod_flush_cbs(_sm);
    end

endmodule
