module aclint_checker
import rv_tester_params:: * ;
#(
        parameter int NUM = -1,
        `TOPOLOGY,
        `RV_TESTER_TRANSACTIONS_ACLINT_CHECKER_OUTPUT_PARAMS
    )(
        input tb_clk,
        input cl_clk,
        input rf_clk,
        input reset,
        input dut_reset,
        input cold_resetn,
        input warm_reset_n,
        input terminated,
        input terminate_now,
        input ac_cr_sync AcCrSynci[NHARTS - 1: 0],
        input rvfi_t[TOTAL_NRETS - 1: 0] rvfi,
        input cr_ac_axi_pkt AcReqPkti,
        input cr_ac_axi_pkt AcReqPktRfClki,
        input logic[63: 0] AcMtimei,
        input logic[8: 0] AcMtipi,
        input logic SmcMtipi,
        input logic AcChk_pll_interrupts_in,
        input logic warm_reset_req,
        input logic [63:0] AcCrCtimeCsr[NHARTS - 1: 0],
        input logic AcCrDebugMode[NHARTS - 1: 0],
        input logic AcCrGateClk[NHARTS - 1: 0],
        input logic pll_shutdown_done,
        `RV_TESTER_TRANSACTIONS_ACLINT_CHECKER_OUTPUT_PORTS
);

    parameter int unsigned location = cvm_topology_gen::get_location (topology.TOP.PLATFORM.ACLINT_CHECKER.ID, 0);
    logic reset_done;
    localparam  DISABLEFUSE = 'h38fff8;
    localparam  MTIME = 'h380000;
    localparam  MTIMECMP0 = 'h388000;
    localparam  TIMESYNC = 'h380018;
    localparam  ACLINT_START = 'h42180000;
    localparam  ACLINT_END = 'h4218ffff;

    logic enable_checks;
    int hart_id[8];
    int nharts = 0;
    bit [63:0] clocks;
    bit time_mtime_sync_enable = 0;

    import "DPI-C" context function void aclint_checker_scope(int unsigned location);
    import "DPI-C" function int get_hart_enable_ids_from_plusargs(output int result[8], input string plusargs_name, int NHARTS);
    always @(posedge tb_clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            enable_checks = cvm_plusargs::get_bool("aclint") != '0;
            $display("SV: ACLINT_CHECKER location %d time %t\n",location,$time);
            aclint_checker_scope(location);
            reset_done = 1'b1;
            // Last argument needs to match the size of the hart_id array
            nharts = get_hart_enable_ids_from_plusargs(hart_id, "hart_enable_id", 8);
            time_mtime_sync_enable = cvm_plusargs::get_bool("time_mtime_sync_enable") != '0;
            /* verilator lint_on BLKSEQ */
        end
        clocks <= clocks + 1;
    end

    //ACLINT force SYNC message checker
    logic forcesynccame;
    assign forcesynccame = (AcReqPktRfClki.addr == TIMESYNC) && AcReqPktRfClki.valid && (AcReqPktRfClki.mask=='hff || AcReqPktRfClki.mask=='hf) && (AcReqPktRfClki.data[7:0] == 8'hff) && (AcReqPktRfClki.user == 3);

    for (genvar n = 0; n < NHARTS; n++) begin : acsync_force
    logic lookout_for_sync;
    always @(posedge rf_clk) begin
        if(dut_reset || AcCrSynci[n].valid) begin
            lookout_for_sync <= 0;
        end else if (forcesynccame) begin
            lookout_for_sync <= 1;
        end
    end
    logic [63:0] count;
    logic violation_forcesync;
    always @(posedge rf_clk) begin
        if (dut_reset || AcCrSynci[n].valid || forcesynccame) begin
            count <= 0;
        end else if (lookout_for_sync) begin
            count <= count + 1;
        end
    end
    assign violation_forcesync =  (count >  'd10) && enable_checks;
    /* verilator lint_off WIDTH */
    assign timesync_checks[n].valid = violation_forcesync;
    assign timesync_checks[n].data.location = location;
    assign timesync_checks[n].data.clock = clocks;
    assign timesync_checks[n].data.hart = n;
    /* verilator lint_on WIDTH */
    end

     /* verilator lint_off MULTIDRIVEN */
    bit clcx_exit_done = 1;
    /* verilator lint_on MULTIDRIVEN */

    always @(posedge pll_shutdown_done) begin
        if (clcx_exit_done) clcx_exit_done <= 0;
    end

    //ACLINT time and mtime synch checker
    for (genvar n = 0; n < NHARTS; n++) begin : time_mtime_synch_checker
    logic violation_time_mtime_synch;
    logic [2:0] compare_counter;
    logic [8:0] reset_counter;
    
    always @(posedge rf_clk) reset_counter [8:0] <= {reset_counter[7:0], warm_reset_n & ~AcCrDebugMode[n] & ~AcCrGateClk[n] & clcx_exit_done};

    always @(posedge rf_clk) begin
        if (!clcx_exit_done && ~AcCrGateClk[n]) begin
            if (AcCrSynci[n].valid) clcx_exit_done <= 1;
        end

        if (!warm_reset_n) begin
            violation_time_mtime_synch <= 0;
            compare_counter <= 0;
        end
        else begin
            if (!(&reset_counter)) violation_time_mtime_synch <= 0; 
            else begin
                if (((AcMtimei - AcCrCtimeCsr[n]) <= 1500) || ((AcCrCtimeCsr[n] - AcMtimei) <= 10)) begin 
                    violation_time_mtime_synch <= 0;
                    compare_counter <= 0;
                end
                else begin
                    compare_counter <= compare_counter + 1;
                    if (compare_counter >= 7) violation_time_mtime_synch <= 1; 
                end
            end
        end
    end
    /* verilator lint_off WIDTH */
    assign time_mtime_synch_checks[n].valid = violation_time_mtime_synch & time_mtime_sync_enable;
    assign time_mtime_synch_checks[n].data.location = location;
    assign time_mtime_synch_checks[n].data.clock = clocks;
    assign time_mtime_synch_checks[n].data.hart = n;
    assign time_mtime_synch_checks[n].data.mtime_data = AcMtimei;
    assign time_mtime_synch_checks[n].data.ctime_data = AcCrCtimeCsr[n];
    /* verilator lint_on WIDTH */
    end

    //ACLINT MTIP generation checker
    typedef enum bit {idle,check} checker_state;
    checker_state [8:0] st;
    logic [8:0] [63:0] counter,counter_check,counter_next, mtimecmpval;
    logic [63:0] counter_mtip8;
    logic [8:0] mtimecmp_wr_valid;
    logic wtimecmp_wr_valid;
    logic mtime_wr_valid;
    /* verilator lint_off WIDTH */

    //ACLINT MTIP generation checker
    bit [8:0] enablefuse;
    bit [3:0] vid [8:0];
    always @(posedge rf_clk) begin
        if(cold_resetn) begin
            for (int i = 0; i < NHARTS ; i++) begin
                if (i < nharts) begin
                    vid[i] <= hart_id[i];
                    enablefuse[i] <= 1;
                end
            end
            vid[8] <= 'd8;
            enablefuse[8] <= 1;
        end
    end

    always_comb begin
        for (int j = 0; j < 9; j++) begin
            // Check if the write request is valid
            mtimecmp_wr_valid[j] = AcReqPktRfClki.valid &&
                                ((AcReqPktRfClki.mask == 'hff && AcReqPktRfClki.addr == MTIMECMP0 + (j << 3)) ||
                                 (AcReqPktRfClki.mask == 'hf && ((AcReqPktRfClki.addr == MTIMECMP0 + (j << 3)) || 
                                                                (AcReqPktRfClki.addr == MTIMECMP0 + (j << 3) + 4))));
        end
    end

    logic [63:0] data_mask;
    assign data_mask = (AcReqPktRfClki.mask == 'hF) ? {32'b0, {32{1'b1}}} : {64{1'b1}};

    logic [63: 0] AcMtimei_delay;
    always @(posedge rf_clk) AcMtimei_delay <= AcMtimei;
    logic [8:0] [63:0] mtimecmp_data;
    logic [63:0] mtime_data;
    assign mtime_data = mtime_wr_valid ? ((AcReqPktRfClki.mask == 'hf) ? {AcMtimei[63:32], AcReqPktRfClki.data[31:0]} : AcReqPktRfClki.data) : AcMtimei;
    generate
    genvar k;
    for ( k = 0; k < 9; k++) begin : mtip_counters
        assign mtimecmp_data[k] = mtimecmp_wr_valid[k] ? ((AcReqPktRfClki.addr == MTIMECMP0 + (k << 3) && AcReqPktRfClki.mask == 'hf) ? {mtimecmpval[k][63:32], AcReqPktRfClki.data[31:0]} :
                                                        (AcReqPktRfClki.addr == MTIMECMP0 + (k << 3) && AcReqPktRfClki.mask == 'hff) ? AcReqPktRfClki.data :
                                                        ((AcReqPktRfClki.addr == MTIMECMP0 + (k << 3) + 4 && AcReqPktRfClki.mask == 'hf) ? {AcReqPktRfClki.data[31:0], mtimecmpval[k][31:0]} :
                                                            mtimecmpval[k])) :
                                                        mtimecmpval[k];
        assign counter_next[k] = mtimecmp_wr_valid[k] ? (mtimecmp_data[k] > AcMtimei ? 64'(mtimecmp_data[k] - AcMtimei) : 64'b0)
                            : mtime_wr_valid ? (mtimecmpval[k] > mtime_data ? 64'(mtimecmpval[k] - mtime_data) : 64'b0)
                            : (AcMtimei < AcMtimei_delay) ? (mtimecmpval[k] > AcMtimei ? 64'(mtimecmpval[k] - AcMtimei) : 64'b0)
                            : (counter[k] < 'd10 ? 64'b0 : 64'(counter[k] - 'd10));
        always @(posedge rf_clk) begin
            if (dut_reset) counter[k] <= 'hffffffff;
            else counter[k] <= counter_next[k];
        end
        always @(posedge rf_clk) begin
            if (dut_reset) mtimecmpval[k] <= 'hffffffff;
            else if(mtimecmp_wr_valid[k]) mtimecmpval[k] <= ((AcReqPktRfClki.mask == 'hf) ? {mtimecmpval[k][63:32], AcReqPktRfClki.data[31:0]} : AcReqPktRfClki.data);
        end

    end
    endgenerate

    // Counter_mtip8 calculation moved outside the generate block
    always @(posedge rf_clk) begin
        counter_mtip8 <= min(counter_next);
    end

    assign mtime_wr_valid = AcReqPktRfClki.valid && AcReqPktRfClki.addr == MTIME && (AcReqPktRfClki.mask=='hff || AcReqPktRfClki.mask=='hf);

    logic [63:0] AcChkMtime;
    always @(posedge rf_clk) begin
        if (!warm_reset_n) AcChkMtime <= 0;
        else begin
           if (mtime_wr_valid) AcChkMtime <= (AcReqPktRfClki.mask == 'h0f) ? {AcChkMtime[63:32], AcReqPktRfClki.data[31:0]} :
                                             (AcReqPktRfClki.mask == 'hf0) ? {AcReqPktRfClki.data[63:32], AcChkMtime[31:0]} :
                                                                              AcReqPktRfClki.data;
           else AcChkMtime <= AcChkMtime + 10; 
        end
    end

    bit AcCrGateClkAny;
    always_comb begin
        AcCrGateClkAny = 1'b0;
        for (int i = 0; i < NHARTS; i++) begin
            AcCrGateClkAny |= AcCrGateClk[i];
        end
    end

    logic AcCrSynci_valid_any;
    always_comb begin
        AcCrSynci_valid_any = 1'b0;
        for (int i = 0; i < NHARTS; i++) begin
            AcCrSynci_valid_any |= AcCrSynci[i].valid;
        end
    end

    logic [63:0] AcChkCtime;
    logic timesync_d;
    always @(posedge rf_clk) begin
        if(dut_reset || AcCrSynci[0].valid) begin
            timesync_d <= 0;
        end else if (forcesynccame) begin
            timesync_d <= 1;
        end
    end
    
    always @(posedge rf_clk) begin
        /* verilator lint_off BLKSEQ */
        if (dut_reset) AcChkCtime <= 0;
        // If the mtime is written, update the ctime
        else if (mtime_wr_valid) 
            AcChkCtime <= (AcReqPktRfClki.mask == 'hf)  ? {AcChkMtime[63:32], AcReqPktRfClki.data[31:0]} :
                          (AcReqPktRfClki.mask == 'hf0) ? {AcReqPktRfClki.data[63:32], AcChkCtime[31:0]} :
                                                          AcReqPktRfClki.data;
        // If sync observed, capture Aclint Checker Mtime to Ctime
        else if (timesync_d && AcCrSynci[0].valid) AcChkCtime <= AcCrSynci[0].data;
        else AcChkCtime <= AcChkCtime;
        /* verilator lint_on BLKSEQ */
    end

    genvar asserti;
    generate
        for ( asserti = 0; asserti < 9; asserti++) begin : mtip_checker
            logic [3:0] coreid;
            logic fail_mtishouldbeON, fail_mtishouldbeOFF;
            assign coreid = enablefuse[asserti] ? vid[asserti] : 4'b1111;
            assign fail_mtishouldbeON = (AcMtipi[coreid] === '0) && (coreid !== 4'b1111) &&  ((coreid === 8) ? counter_mtip8 == 0 : (counter[asserti] === 0 && enablefuse[asserti]));
            assign fail_mtishouldbeOFF = (AcMtipi[coreid] === '1) && (coreid !== 4'b1111) && ~((coreid === 8) ? counter_mtip8 == 0 : (counter[asserti] === 0 && enablefuse[asserti]));

            logic [4:0] cycles_in_fail_mtishouldbeON [8:0];
            logic [4:0] cycles_in_fail_mtishouldbeOFF [8:0];
            always @(posedge rf_clk) begin
                if(dut_reset || ~fail_mtishouldbeON) cycles_in_fail_mtishouldbeON[asserti] <= 0;
                else if(fail_mtishouldbeON) cycles_in_fail_mtishouldbeON[asserti] <= cycles_in_fail_mtishouldbeON[asserti] + 1;
            end
            always @(posedge rf_clk) begin
                if(dut_reset || ~fail_mtishouldbeOFF) cycles_in_fail_mtishouldbeOFF[asserti] <= 0;
                else if(fail_mtishouldbeOFF) cycles_in_fail_mtishouldbeOFF[asserti] <= cycles_in_fail_mtishouldbeOFF[asserti] + 1;
            end
            // always_comb assert(~(cycles_in_fail_mtishouldbeOFF[asserti] > 5)) else $error("[%0d] Error: Did not expect MTIP, but MTIP %d generated", clocks, asserti);
            // always_comb assert(~(cycles_in_fail_mtishouldbeON[asserti] > 5)) else $error("[%0d] Error: Expected MTIP, but MTIP %d not generated", clocks, asserti);
            assign mtip_checks[asserti].valid         = ((cycles_in_fail_mtishouldbeOFF[asserti] > 5) || (cycles_in_fail_mtishouldbeON[asserti] > 5)); 
            assign mtip_checks[asserti].data.location = location;
            assign mtip_checks[asserti].data.check_1  = (cycles_in_fail_mtishouldbeOFF[asserti] > 5);
            assign mtip_checks[asserti].data.check_2  = (cycles_in_fail_mtishouldbeON[asserti] > 5);
            assign mtip_checks[asserti].data.clock    = clocks;
            assign mtip_checks[asserti].data.hart     = asserti;
        end
    endgenerate

    int max_mtip8_delay = 5;
    int mtip8_delay_counter;

    always @(posedge rf_clk) begin
        if (dut_reset) mtip8_delay_counter <= 0;
        else if(AcMtipi[8] & ~SmcMtipi) mtip8_delay_counter <= mtip8_delay_counter + 1;
        else mtip8_delay_counter <= 0;
    end
    always_comb assert (mtip8_delay_counter < max_mtip8_delay) else $error("[%0d] Error: MTIP[8] generated by Aclint but not seen at SMC port", clocks);

    //ACLINT core MMR - ac_mmrwrite
    for (genvar n = 0; n < TOTAL_NRETS; n++) begin
        assign cr_ac_mmrwrites[n].valid =  warm_reset_n & enable_checks & rvfi[n].valid && (rvfi[n].mode == 3) && (rvfi[n].mem_wmask != 0) && (!rvfi[n].vec) && (rvfi[n].mem_paddr >= ACLINT_START && rvfi[n].mem_paddr < ACLINT_END);
        assign cr_ac_mmrwrites[n].data.location = location;
        assign cr_ac_mmrwrites[n].data.addr = rvfi[n].mem_paddr;
        assign cr_ac_mmrwrites[n].data.mask = rvfi[n].mem_wmask;
        assign cr_ac_mmrwrites[n].data.data = rvfi[n].mem_wdata[63:0];
    end

        assign axi_ac_writes[0].valid = AcReqPkti.valid & enable_checks;
        assign axi_ac_writes[0].data.location = location;
        assign axi_ac_writes[0].data.srcid = AcReqPkti.srcid;
        assign axi_ac_writes[0].data.addr = topology.TOP.PLATFORM.PALEN'(AcReqPkti.addr);
        assign axi_ac_writes[0].data.data = topology.TOP.PLATFORM.XLEN'(AcReqPkti.data);
        assign axi_ac_writes[0].data.mask = AcReqPkti.mask;
        assign axi_ac_writes[0].data.user = AcReqPkti.user;
    /* verilator lint_on WIDTH */

    function longint unsigned get_mtime_value();
        return AcChkMtime;
    endfunction
    export "DPI-C" function get_mtime_value;

    function longint unsigned get_ctime_value();
        return AcChkCtime;
    endfunction
    export "DPI-C" function get_ctime_value;

    import "DPI-C" function void check_outstanding_transactions(int unsigned location, longint unsigned clocks);
    import "DPI-C" function void clear_core_outstanding_transactions(int unsigned location);
    import "DPI-C" function void time_mtime_eot_error();
    always @(posedge tb_clk) begin
        if ($rose(warm_reset_req) || $fell(warm_reset_n)) clear_core_outstanding_transactions(location);
        // dut.cpl_top0.i_pll_controller.pll_interrupts_in leads to pll_shutdown leading to trigger terminate sequence
        // Destroying any transaction that is inflight. Thus ignoring checks when terminate is asserted due to the same. 
        else if (!reset && !AcChk_pll_interrupts_in && enable_checks && terminate_now && !terminated) begin
            check_outstanding_transactions(location, clocks);
            if (~clcx_exit_done && ~AcCrGateClkAny && time_mtime_sync_enable) time_mtime_eot_error();
        end
    end

  function automatic logic [3:0] get_hart_ret(int n);
    logic [3:0] hart;
    /* verilator lint_off WIDTH */
    hart = NHARTS - 1;
    for (int i=0; i < NHARTS-1; i++) begin
      if (n >= NRETS_CUMSUM[i] && n < NRETS_CUMSUM[i+1]) begin
        hart = i;
        break;
      end
    end
    /* verilator lint_on WIDTH */
    return hart;
  endfunction

  function automatic logic [3:0] get_hart_bypass(int n);
    logic [3:0] hart;
    /* verilator lint_off WIDTH */
    hart = NHARTS - 1;
    for (int i=0; i < NHARTS-1; i++) begin
      if (n >= NBYPASSES_CUMSUM[i] && n < NBYPASSES_CUMSUM[i+1]) begin
        hart = i;
        break;
      end
    end
    /* verilator lint_on WIDTH */
    return hart;
  endfunction

  function automatic logic [63:0] min(logic [8:0] [63:0] arr);
    min = arr[0];
    for (int i = 1; i < 9; i++) min = (arr[i] < min) ? arr[i] : min;
  endfunction 

endmodule
