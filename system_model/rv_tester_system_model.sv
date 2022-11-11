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

    import "DPI-C" function chandle rv_tester_system_model_new();

    chandle _sm = null;
    function chandle init();
        if (_sm == null) begin
            _sm = rv_tester_system_model_new();
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

    import "DPI-C" function void clock_update(longint unsigned clocks);
    always @(posedge clk) begin
        if (0 == (clocks % (SW_CLOCK_UPDATE_PERIOD/CLOCK_PERIOD))) begin
            clock_update(clocks);
        end
    end

    rv_tester_pkg::interrupt_t interrupt_d, interrupt_q;
    assign interrupt = interrupt_q;

    function void set_interrupt(byte unsigned is_timer);
        /* verilator lint_off BLKANDNBLK */
        if (is_timer != 0) interrupt_d.timer    = '1;
        else               interrupt_d.external = '1;
        /* verilator lint_on BLKANDNBLK */
    endfunction

endmodule
