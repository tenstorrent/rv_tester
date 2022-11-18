module rv_tester_system_model #(
    parameter int  RESET_CLOCKS           =    10,
    parameter time CLOCK_PERIOD           = 500ps,
    parameter time SW_CLOCK_UPDATE_PERIOD = 100ns,
    rv_tester_pkg::cfg_t CFG              =    '0,
    `RV_TESTER_PARAMETERS(CFG)
)(
    input clk,
    output reset,
    output bootstrap_t bootstrap,
    output rv_tester_pkg::interrupt_t interrupt
);
    import "DPI-C" context function chandle sysmod_new(string memmap);
    import "DPI-C" function void sysmod_load_program(chandle sysmod_p, string prog);
    import "DPI-C" function void sysmod_tick(chandle sysmod_p, longint unsigned advance);
    import "DPI-C" context function void sysmod_flush_cbs(chandle sysmod_p);

    chandle _sm = null;
    string prog;
    function chandle init();
        if (_sm == null) begin
            _sm = sysmod_new("memmap.json");
        end
        if ($value$plusargs("hex=%s", prog)) begin
          sysmod_load_program(_sm, prog);
        end
        return _sm;
    endfunction

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

    always @(posedge clk) begin
        // FIXME: should be queued up in separate thread
        if (0 == (clocks % (SW_CLOCK_UPDATE_PERIOD/CLOCK_PERIOD))) begin
            sysmod_tick(_sm, (SW_CLOCK_UPDATE_PERIOD/CLOCK_PERIOD));
        end
    end

    rv_tester_pkg::interrupt_t interrupt_d, interrupt_q;
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
        interrupt_q <= interrupt_d;

        // FIXME: add poll
        sysmod_flush_cbs(_sm);
    end

endmodule
