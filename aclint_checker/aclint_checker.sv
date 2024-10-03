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
        input bit terminate,
        input ac_cr_sync AcCrSynci[NHARTS - 1: 0],
        input rvfi_t[TOTAL_NRETS - 1: 0] rvfi,
        input mcmi_t[TOTAL_NBYPASSES - 1: 0] mcmi_bypass,
        input cr_ac_axi_pkt AcReqPkti,
        input cr_ac_axi_pkt AcReqPktRfClki,
        input logic[63: 0] AcMtimei,
        input logic[8: 0] AcMtipi,
        `RV_TESTER_TRANSACTIONS_ACLINT_CHECKER_OUTPUT_PORTS
);

    parameter int unsigned location = cvm_topology_gen::get_location (topology.TOP.PLATFORM.ACLINT_CHECKER.ID, 0);
    logic reset_done;
    localparam  WAKECORE = 'h380010;
    localparam  WAKETIME = 'h380008;
    localparam  MTIMECMP0 = 'h388000;
    localparam  TIMESYNC = 'h380018;
    localparam  ACLINT_START = 'h42180000;
    localparam  ACLINT_END = 'h4218ffff;
    logic enable_checks;

    always @(posedge tb_clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            enable_checks = cvm_plusargs::get_bool("aclint") != '0;
            if (enable_checks)
            $display("SV: ACLINT_CHECKER location %d time %t\n",location,$time);
            /* verilator lint_on BLKSEQ */
            /* verilator lint_off BLKSEQ */
            reset_done = 1'b1;
            /* verilator lint_on BLKSEQ */
        end
    end

    //ACLINT force SYNC message checker
    logic forcesynccame;
    assign forcesynccame = (AcReqPktRfClki.addr == TIMESYNC) && AcReqPktRfClki.valid && AcReqPktRfClki.mask=='hff && (AcReqPktRfClki.data == 'hff);

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
    assign violation_forcesync =  (count >  'd4) && enable_checks ;
    always_comb
    assert (~violation_forcesync) else $error("Error: Not recieved aclint force sync");
    end

    //ACLINT MTIP generation checker
    typedef enum bit {idle,check} checker_state;
    checker_state [8:0] st;
    logic [8:0] [63:0] counter,counter_check;
    logic [8:0] mtimecmp_wr_valid;

    /* verilator lint_off WIDTH */
    always @(posedge rf_clk) begin
        for (int j = 0; j < 9; j++) begin
        if (dut_reset || AcMtipi[j] || mtimecmp_wr_valid[j]) begin
            counter[j] <= 0;
        end else begin
            counter[j] <= counter[j]+1;
        end
        if (dut_reset || AcMtipi[j] || ~enable_checks) begin
            st[j] <= idle;
            counter_check[j] <= 'hffffffff ;
        end else if (mtimecmp_wr_valid[j]) begin
            st[j] <= check;
            counter_check[j] <= AcReqPktRfClki.data > AcMtimei ? AcReqPktRfClki.data - AcMtimei : 0;
        end
        end
    end
    genvar asserti;
    generate
    for ( asserti = 0; asserti < 9; asserti++) begin : mtip_check
    always_comb
    assert(~((counter[asserti] > counter_check[asserti]) && (st[asserti] == check) && (counter[asserti]-counter_check[asserti]) > 4)) else $error("Error: Expected MTIP, but MTIP not generated");
    end
    endgenerate

    logic [63:0] wakecore;
    always @(posedge rf_clk) begin
        if(dut_reset) begin
            wakecore <= 0;
        end else if ((AcReqPktRfClki.addr == WAKECORE) && AcReqPktRfClki.valid && AcReqPktRfClki.mask=='hff) begin
            wakecore <= AcReqPktRfClki.data;
        end
    end
    always_comb begin
        for (int j = 0; j < 9; j++) begin
            mtimecmp_wr_valid[j] = AcReqPktRfClki.valid && AcReqPktRfClki.mask=='hff && ( (AcReqPktRfClki.addr == (MTIMECMP0 + (j<<3) )) || ((AcReqPktRfClki.addr == WAKETIME ) && wakecore==j) );
        end
    end


    //ACLINT core MMR - ac_mmrwrite
    for (genvar n = 0; n < TOTAL_NRETS; n++) begin
        assign cr_ac_mmrwrites[n].valid =  ~reset & enable_checks & rvfi[n].valid && (rvfi[n].mem_wmask != 0) && (rvfi[n].mem_paddr>= ACLINT_START && rvfi[n].mem_paddr< ACLINT_END);
        assign cr_ac_mmrwrites[n].data.location = location;
        assign cr_ac_mmrwrites[n].data.hart = get_hart_ret(n);
        assign cr_ac_mmrwrites[n].data.order = rvfi[n].order;
        assign cr_ac_mmrwrites[n].data.addr = rvfi[n].mem_paddr;
        assign cr_ac_mmrwrites[n].data.mask = rvfi[n].mem_wmask;
        assign cr_ac_mmrwrites[n].data.data = rvfi[n].mem_wdata[63:0];
    end

    for (genvar n = 0; n < TOTAL_NBYPASSES; n++) begin
        assign cr_ac_mmrwr_bypasss[n].valid =   enable_checks & mcmi_bypass[n].valid && (mcmi_bypass[n].mask != 0) && (mcmi_bypass[n].addr>= ACLINT_START && mcmi_bypass[n].addr< ACLINT_END);
        assign cr_ac_mmrwr_bypasss[n].data.location = location;
        assign cr_ac_mmrwr_bypasss[n].data.hart = get_hart_bypass(n);
        assign cr_ac_mmrwr_bypasss[n].data.order = mcmi_bypass[n].order;
        assign cr_ac_mmrwr_bypasss[n].data.mask = mcmi_bypass[n].mask;
        assign cr_ac_mmrwr_bypasss[n].data.data = mcmi_bypass[n].data[63:0];
    end

        assign ac_axi_writes[0].valid =   enable_checks & AcReqPkti.valid;
        assign ac_axi_writes[0].data.location = location;
        assign ac_axi_writes[0].data.hart = AcReqPkti.srcid;
        assign ac_axi_writes[0].data.addr = topology.TOP.PLATFORM.PALEN'(AcReqPkti.addr);
        assign ac_axi_writes[0].data.data = topology.TOP.PLATFORM.XLEN'(AcReqPkti.data);
        assign ac_axi_writes[0].data.mask = AcReqPkti.mask;
    /* verilator lint_on WIDTH */

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




endmodule
