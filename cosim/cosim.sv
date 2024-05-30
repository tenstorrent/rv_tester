module cosim
import rv_tester_params::*;
#(
    parameter int NUM = -1,
    parameter int NRET = 1,
    parameter int NREAD = 1,
    parameter int NINSERT = 1,
    parameter int NWRITE = 1,
    parameter int NBYPASS = 1,
    parameter int NIFETCH = 1,
    parameter int NIEVICT = 1,
    parameter int RESET_CLOCKS = 10,
    parameter int MAX_CSR_AFTER_NRET = 3,
    `TOPOLOGY,
    `RV_TESTER_TRANSACTIONS_COSIM_OUTPUT_PARAMS
)(
    input tb_clk,
    input clk,
    input reset,
    input dut_reset,
    input longint unsigned clocks,
    input rvfi_t [NRET-1:0] rvfi,
    input csri_t csri,
    input mcmi_t [NREAD-1:0] mcmi_read,
    input mcmi_t [NINSERT-1:0] mcmi_insert,
    input mcmi_t [NWRITE-1:0] mcmi_write,
    input mcmi_t [NBYPASS-1:0] mcmi_bypass,
    input mcmi_t [NIFETCH-1:0] mcmi_ifetch_req,
    input mcmi_t [NIFETCH-1:0] mcmi_ifetch_resp,
    input mcmi_t [NIEVICT-1:0] mcmi_ievict,
    input rv_tester_pkg::interrupt_t wired_interrupt,
    input rv_tester_params::mst_req_top imsic_interrupt,
    input debug_mode,
    input longint eot_addr,
    output rv_tester_pkg::terminate_t terminate,
    `RV_TESTER_TRANSACTIONS_COSIM_OUTPUT_PORTS
);


localparam CSR_SBITS = $clog2(CSR_COUNT);
localparam MAXCSR = NRET + MAX_CSR_AFTER_NRET;
localparam NGP_REGS  = 32; 
localparam NFP_REGS  = 32; 
localparam NVC_REGS  = 32;
localparam PA_WIDTH  = $size(rvfi[0].mem_paddr);
localparam GP_WIDTH  = $size(rvfi[0].rd_wdata);
localparam FP_WIDTH  = $size(rvfi[0].frd_wdata);
localparam VC_WIDTH  = $size(rvfi[0].vrd_wdata);
localparam MCM_AWIDTH  = $size(mcmi_write[0].addr);

    //----------------------------------------------------------------------------
    // function retsel compresses CSR_COUNT down into MAXCSR+1 DPI calls
    //   we make the retsel function have MAXCSR+1 values to catch if we have too
    //   many CSR updates.  If s=MAXCSR then we went past the number DPIs we can
    //   accomodate  (0 .. MAXCSR-1)
    //----------------------------------------------------------------------------
    
    function automatic bit [MAXCSR:0][CSR_SBITS-1:0] retsel(input bit [CSR_COUNT-1:0] valid);
        int s = 0;
        int i = 0;
        retsel = '1;
        /* verilator lint_off WIDTH */
        for(i=0;i<CSR_COUNT;i=i+1) begin
            if (valid[i] == 1) begin
                retsel[s] = i;
                if (s < MAXCSR) begin
                    s = s + 1;
                end
            end
        end
        /* verilator lint_on WIDTH */
    endfunction
    
    function automatic [$clog2(NRET+1)-1:0] count(input bit [NRET-1:0] valid, input bit [NRET-1:0][63:0] order);
        bit [63:0] corder=0; 
        count = 0;
        /* verilator lint_off WIDTH */
        for(int i=0;i<NRET;i=i+1) begin
            if (i==0) begin
                if (valid[0] == 1) begin
                   count = 1;
                end
            end
            else begin
                if ((valid[i] == 1) & ((order[i] != order[i-1]) )) begin
                   count = count + 1; 
                end
            end
        end
        /* verilator lint_on WIDTH */
    endfunction

    function automatic [64:0] hipri_wdata64(input bit [4:0] rsel, input bit [NRET-1:0] valid, input bit [NRET-1:0][4:0] rd_addr, input bit [NRET-1:0][63:0] wdata);
        /* verilator lint_off WIDTH */
        for(int i=NRET-1;i>=0;i=i-1) begin
            if (valid[i] == 1'b1) begin
                if (rd_addr[i] == rsel) begin
                   return({1'b1,wdata[i]});
                end
            end
        end
        return({1'b0,64'h0});
        /* verilator lint_on WIDTH */
    endfunction
    function automatic [256:0] hipri_wdata256(input bit [4:0] rsel, input bit [NRET-1:0] valid, input bit [NRET-1:0][4:0] rd_addr, input bit [NRET-1:0][255:0] wdata);
        /* verilator lint_off WIDTH */
        for(int i=NRET-1;i>=0;i=i-1) begin
            if (valid[i] == 1'b1) begin
                if (rd_addr[i] == rsel) begin
                   return({1'b1,wdata[i]});
                end
            end
        end
        return({1'b0,256'h0});
        /* verilator lint_on WIDTH */
    endfunction

    import "DPI-C" context function void cosim_set_scope(int unsigned location);


    typedef longint unsigned LU;
    int unsigned location = cvm_topology::nil;
    bit rvfi_enabled;
    bit state_cmp_enabled;
    int unsigned scheck_period=0;

    bit get_cosim_compare_values = 1;
    bit reset_d1 = 1;
    bit [NGP_REGS-1:0][4:0]                    rd_addr;               //register-retire load enable
    bit [NGP_REGS-1:0][4:0]                    frd_addr;              //register-retire load enable
    bit [NGP_REGS-1:0][4:0]                    vrd_addr;              //register-retire load enable
    bit [NGP_REGS-1:0]                         rd_load;               //register-retire load enable
    bit [NFP_REGS-1:0]                         frd_load;
    bit [NVC_REGS-1:0]                         vrd_load;
    bit [NGP_REGS-1:0][GP_WIDTH-1:0]           srd_wdata;              //retister-retire write data
    bit [NFP_REGS-1:0][FP_WIDTH-1:0]           sfrd_wdata;              //NRET entry is used for feedback of current value
    bit [NVC_REGS-1:0][VC_WIDTH-1:0]           svrd_wdata;
    bit [NGP_REGS-1:0][GP_WIDTH-1:0]           rd_wdata,  gp_wdata_in, gp_regs;
    bit [NFP_REGS-1:0][FP_WIDTH-1:0]           frd_wdata, fp_wdata_in, fp_regs;
    bit [NVC_REGS-1:0][VC_WIDTH-1:0]           vrd_wdata, vc_wdata_in, vc_regs;
    bit                                        gp_reg_written;
    bit                                        fp_reg_written;
    bit                                        vc_reg_written;

    bit [NWRITE-1:0]  mcmi_write_pokes;
    bit               mcmi_write_poke;
    bit [NBYPASS-1:0] mcmi_bypass_pokes;
    bit               mcmi_bypass_poke;
    longint unsigned scheck_addr_hi=0;
    longint unsigned scheck_addr_lo=0;

    bit [31:0]             rvfi_scheck_cnt;
    bit [NRET-1:0]         rvfi_valids;
    bit [NRET-1:0][63:0]   rvfi_orders;
    bit [NRET-1:0]         rvgp_valids;
    bit [NRET-1:0]         rvfp_valids;
    bit [NRET-1:0]         rvvc_valids;
    bit [NRET-1:0]         sc_rw;
    bit [NRET-1:0]         csr_rw;
    bit [NRET-1:0]         excepts;
    bit [NRET-1:0]         intr_memw;
    bit [NRET-1:0]         cmp_memw;
    bit [NRET-1:0]         poke_events;                     // events that should cause a Poke in Whisper 
    bit [NWRITE-1:0]       eot_writes_found;                // end-of-test event found in mcmi_writes ifc
    bit [NBYPASS-1:0]      eot_bypass_found;                // end-of-test event found in mcmi_bypass ifc
    bit [NRET-1:0]         force_steps;                     // At end-of-test if "steps" need to executed we can force bridge to do them 
    bit [31:0]             rvfi_steps;                      // total number of rvfi_valids we did NOT send 
    bit [$clog2(NRET+1)-1:0] valid_cnt;                      // number of rvfi_valids == 1 in 1 clock

    bit                    eot_found;                       // end-of-test event found 
    bit                    eot_max_instr;                   // max # instructions end-of-test event found 
    bit                    rvfi_valid;
    bit                    send_rvfi;
    bit                    compare_state;
    bit                    gp_regs_changed, gp_changed; 
    bit                    fp_regs_changed, fp_changed; 
    bit                    vc_regs_changed, vc_changed; 

    // Timeout checks
    int max_stall_cycle = 50000;
    longint unsigned max_cycle;
    bit [31:0] max_instructions;
    bit [31:0] instruction_cnt;
    int cycles_since_retire;
    longint unsigned hart_enable_mask;
    bit boot_wfi;

    for(genvar n=0;n<NGP_REGS;n=n+1) begin
        assign {rd_load[n], srd_wdata[n]}  = hipri_wdata64( n, rvgp_valids, rd_addr, rd_wdata);
        assign {frd_load[n],sfrd_wdata[n]} = hipri_wdata64( n, rvfp_valids, frd_addr, frd_wdata);
        if (VC_WIDTH==256) 
            assign {vrd_load[n],svrd_wdata[n]} = hipri_wdata256(n, rvvc_valids, vrd_addr, vrd_wdata);
        else
            assign {vrd_load[n],svrd_wdata[n]} = hipri_wdata64(n, rvvc_valids, vrd_addr, vrd_wdata);
        assign gp_wdata_in[n] = (rd_load[n]==1'b1)  ? srd_wdata[n]  : gp_regs[n];
        assign fp_wdata_in[n] = (frd_load[n]==1'b1) ? sfrd_wdata[n] : fp_regs[n];
        assign vc_wdata_in[n] = (vrd_load[n]==1'b1) ? svrd_wdata[n] : vc_regs[n];
    end

    for(genvar n=0;n<NRET;n=n+1) begin
        assign rvfi_valids[n] = rvfi[n].valid ;
        assign rvfi_orders[n] = rvfi[n].order ;
        assign rd_addr[n]     = rvfi[n].rd_addr[4:0];
        assign frd_addr[n]    = rvfi[n].frd_addr[4:0];
        assign vrd_addr[n]    = rvfi[n].vrd_addr[4:0];
        assign rd_wdata[n]    = rvfi[n].rd_wdata;
        assign frd_wdata[n]   = rvfi[n].frd_wdata;
        assign vrd_wdata[n]   = rvfi[n].vrd_wdata;
        assign rvgp_valids[n] = rvfi[n].valid & (rvfi[n].rd_addr != 0) ? 1'b1 : 1'b0;
        assign rvfp_valids[n] = rvfi[n].valid & rvfi[n].frd_valid;
        assign rvvc_valids[n] = rvfi[n].valid & rvfi[n].vrd_valid;
    end

    assign rvfi_valid = | rvfi_valids;                        // OR of all valid retires


    //------------------------------------------------------------------------------------------------------------------
    // FORCE STEPS logic:
    //   When the EOT is found we and we have UN-accounted for whisper steps to execute we need
    //   send these immediately (poke event).
    //   ISSUE: there may or may not be an RVFI valid to piggyback on to to issue the steps
    //   FIX:   we will FORCE valid 0 to be high and set the "force_steps" flag to 1
    //          force steps will tell the bridge that there are no instructions begin sent... just STEPS to execute
    //          If there IS an instruction retiring we will not force the steps
    //------------------------------------------------------------------------------------------------------------------

    assign force_steps[0] = (~rvfi_valid & eot_found & (rvfi_steps != 0)) ? 1'b1: 1'b0;  
    for(genvar n=1;n<NRET;n=n+1) begin
        assign force_steps[n] = 1'b0; 
    end


    //--------------------------------------------------------------------------------------------
    // Track writes to GP,FP,VEC registers for comparison with Whisper 
    //--------------------------------------------------------------------------------------------
    for(genvar r=0;r<NGP_REGS;r=r+1) begin
        always @(posedge clk) begin
            if (dut_reset==1'b1)  begin
                gp_regs[r] <= '0;
                fp_regs[r] <= '0;
                vc_regs[r] <= '0;
            end
            else begin
                gp_regs[r] <= gp_wdata_in[r]; 
                fp_regs[r] <= fp_wdata_in[r]; 
                vc_regs[r] <= vc_wdata_in[r]; 
            end
        end
    end

    assign gp_reg_written = (rvgp_valids != 0);
    assign fp_reg_written = (rvfp_valids != 0);
    assign vc_reg_written = (rvvc_valids != 0);

    always @(posedge clk) begin
        if (dut_reset==1'b1)  
            gp_regs_changed <= 1'b0;
        else
        if (compare_state) 
            gp_regs_changed <= 1'b0;                  // clear change register...unless we are writting in the save state too
        else
        if (gp_reg_written)                           // not saving the state..set if writting a GP register 
            gp_regs_changed <= 1'b1;
    end

    always @(posedge clk) begin
        if (dut_reset==1'b1)  
            fp_regs_changed <= 1'b0;
        else
        if (compare_state) 
            fp_regs_changed <= 1'b0;                  // clear change register...unless we are writting in the save state too
        else
        if (fp_reg_written)                           // not saving the state..set if writting a FP register 
            fp_regs_changed <= 1'b1;
    end

    always @(posedge clk) begin
        if (dut_reset==1'b1)  
            vc_regs_changed <= 1'b0;
        else
        if (compare_state) 
            vc_regs_changed <= 1'b0;                  // clear change register...unless we are writting in the save state too
        else
        if (vc_reg_written)                           // not saving the state..set if writting a vector register 
            vc_regs_changed <= 1'b1;
    end

    assign gp_changed = gp_regs_changed | gp_reg_written;
    assign fp_changed = fp_regs_changed | fp_reg_written;
    assign vc_changed = vc_regs_changed | vc_reg_written;


    //---------------------------------------------------------------------------------
    // State Compare method enabled with SCHECK_PERIOD set to value > 0
    //    - when method not enabled we should operate in legacy mode
    //---------------------------------------------------------------------------------
    assign state_cmp_enabled = (scheck_period != 0);


    //---------------------------------------------------------------------------------
    // POKE EVENTS logic
    //   - Several cases where we need to send the RVFI packet...
    //        1) SC instruction decoded
    //        2) CSR instruction decoded
    //        3) Execptions 
    //        4) Interrupts 
    //        5) Interrupts Causes 
    //        6) End-Of-Test detected 
    //
    //   - We force an RVFI to be sent when we need to do a SAVE STATES
    //       - these only occur when there is an RVFI valid=1
    //---------------------------------------------------------------------------------
    for(genvar n=0;n<NRET;n=n+1) begin
        assign sc_rw[n]  = rvfi[n].valid & ((rvfi[n].insn[6:0] == 7'b0101111) & (rvfi[n].insn[14:13] == 2'b01)  & (rvfi[n].insn[31:27] == 5'b00011));
        assign csr_rw[n] = rvfi[n].valid & ((rvfi[n].insn[6:0] == 7'b1110011) & (rvfi[n].insn[14:12] != 3'b000));
        assign excepts[n]= rvfi[n].valid & (rvfi[n].intr  | rvfi[n].trap);
        //if (d.mem_write.valid && d.mem_write.size==4 && ((d.mem_write.pa>=0x40000000 &&  d.mem_write.pa <0x42000000) || (d.mem_write.pa>=0x44000000 &&  d.mem_write.pa < 0x46000000)) ) {
        assign intr_memw[n]= rvfi[n].valid & (rvfi[n].mem_wmask != 0) & (rvfi[n].mem_wmask[1:0]==2'b11) & (rvfi[n].mem_paddr[PA_WIDTH-1:32] == '0) & ((rvfi[n].mem_paddr[31:25]==7'h20) | (rvfi[n].mem_paddr[31:25]==7'h22));

        assign cmp_memw[n]= rvfi[n].valid & (rvfi[n].mem_wmask != 0) & (rvfi[n].mem_paddr[PA_WIDTH-1:0] >= scheck_addr_lo) & (rvfi[n].mem_paddr[PA_WIDTH-1:0] <= scheck_addr_hi);

        assign poke_events[n]  = (sc_rw[n] | csr_rw[n] | excepts[n] | intr_memw[n] | cmp_memw[n] | compare_state) | ~state_cmp_enabled; 
    end

    assign send_rvfi = | poke_events;
 

    
    assign m_gp_regss[0].valid      = compare_state;
    assign m_gp_regss[0].data.hart  = NUM;
    assign m_gp_regss[0].data.location = location;
    for(genvar n=0;n<NGP_REGS;n=n+1) begin
        assign m_gp_regss[0].data.value[n][63:0] = gp_wdata_in[n];
    end

    assign m_fp_regss[0].valid = compare_state & fp_regs_changed;
    assign m_fp_regss[0].data.hart  = NUM;
    assign m_fp_regss[0].data.location = location;
    for(genvar n=0;n<NFP_REGS;n=n+1) begin
        assign m_fp_regss[0].data.value[n][63:0] = fp_wdata_in[n];
    end

    assign m_vc_regss[0].valid = compare_state & vc_regs_changed;
    assign m_vc_regss[0].data.hart  = NUM;
    assign m_vc_regss[0].data.location = location;
    for(genvar n=0;n<NVC_REGS;n=n+1) begin
        assign m_vc_regss[0].data.value[n] = vc_wdata_in[n];
    end


    always @(posedge tb_clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            location = cvm_topology::get_location(topology.TOP.PLATFORM.COSIM.ID, NUM);
            rvfi_enabled = (cvm_plusargs::get_bool("rvfi") != '0) & (location != cvm_topology::nil);
            if (rvfi_enabled) begin
              cosim_set_scope(location);
            end
            terminate.terminate = '0;
            /* verilator lint_on BLKSEQ */
        end
    end

    function void cosim_terminate ();
        $display("[cosim]: attempting to terminate");
        /* verilator lint_off BLKSEQ */
        terminate.terminate = '1;
        /* verilator lint_on BLKSEQ */
    endfunction


    //--------------------------------------------------------------------------------------------------------
    // force_steps == 1 ONLY for rvfi[0].  Only asserted when we need to advance the model but there are no
    //   valid rvfi packets to piggyback on so we force rvfi pkt 0 to go in... but ONLY to step the ISS model
    //--------------------------------------------------------------------------------------------------------

    // m_rvfi
    for (genvar n = 0; n < NRET; n++) begin
        assign m_rvfis[n].valid            = RVFI_EN & rvfi_enabled & ~dut_reset & (rvfi[n].valid | force_steps[n]) & send_rvfi;
        assign m_rvfis[n].data.location    = location;
        assign m_rvfis[n].data.cycle       = clocks;
        assign m_rvfis[n].data.steps       = rvfi_steps;
        assign m_rvfis[n].data.step_only   = force_steps[n]; 
        assign m_rvfis[n].data.hart        = NUM;
        assign m_rvfis[n].data.last_uop    = rvfi[n].last_uop;
        assign m_rvfis[n].data.last_insn   = rvfi[n].last_insn;
        assign m_rvfis[n].data.comp        = rvfi[n].comp;
        assign m_rvfis[n].data.order       = rvfi[n].order;
        assign m_rvfis[n].data.insn        = rvfi[n].insn;
        assign m_rvfis[n].data.uop         = rvfi[n].uop;
        assign m_rvfis[n].data.trap        = rvfi[n].trap;
        assign m_rvfis[n].data.cause       = rvfi[n].cause;
        assign m_rvfis[n].data.intr        = rvfi[n].intr;
        assign m_rvfis[n].data.mode        = rvfi[n].mode;
        assign m_rvfis[n].data.vec_cracked = rvfi[n].vec_cracked;
        assign m_rvfis[n].data.ixl         = rvfi[n].ixl;
        assign m_rvfis[n].data.rd_addr     = rvfi[n].rd_addr;
        assign m_rvfis[n].data.rd_wdata    = rvfi[n].rd_wdata;
        assign m_rvfis[n].data.frd_valid   = rvfi[n].frd_valid;
        assign m_rvfis[n].data.frd_addr    = rvfi[n].frd_addr;
        assign m_rvfis[n].data.frd_wdata   = rvfi[n].frd_wdata;
        assign m_rvfis[n].data.vrd_valid   = rvfi[n].vrd_valid;
        assign m_rvfis[n].data.vrd_addr    = rvfi[n].vrd_addr;
        assign m_rvfis[n].data.vrd_wdata   = rvfi[n].vrd_wdata;
        assign m_rvfis[n].data.csr_valid   = rvfi[n].csr_valid;
        assign m_rvfis[n].data.csr_addr    = rvfi[n].csr_addr;
        assign m_rvfis[n].data.csr_wdata   = rvfi[n].csr_wdata;
        assign m_rvfis[n].data.csr_wmask   = rvfi[n].csr_wmask;
        assign m_rvfis[n].data.pc_rdata    = rvfi[n].pc_rdata;
        assign m_rvfis[n].data.pc_wdata    = rvfi[n].pc_wdata;
        assign m_rvfis[n].data.mem_addr    = rvfi[n].mem_addr;
        assign m_rvfis[n].data.mem_paddr   = rvfi[n].mem_paddr;
        assign m_rvfis[n].data.mem_rmask   = rvfi[n].mem_rmask;
        assign m_rvfis[n].data.mem_rdata   = rvfi[n].mem_rdata;
        assign m_rvfis[n].data.mem_wmask   = rvfi[n].mem_wmask;
        assign m_rvfis[n].data.mem_wdata   = rvfi[n].mem_wdata;
        assign m_rvfis[n].data.mem_attr    = rvfi[n].mem_attr;


    end   // for loop n

    //---------------------------------------------------------------
    // Keep track of number of instructions we did NOT send to cosim
    //  steps will be used to advance the Whisper model 
    //---------------------------------------------------------------

    assign valid_cnt = count(rvfi_valids, rvfi_orders);

    always @(posedge clk) begin
        if (dut_reset) begin
            rvfi_steps   <= '0; 
            rvfi_scheck_cnt <= '0; 
        end
        else begin
            if (rvfi_valid) begin                                    // instruction(s) retiring 
                if (~send_rvfi)                                      // we need to send a valid but we chose NOT to
                    rvfi_steps <= rvfi_steps + valid_cnt;            // increment how many valids we did NOT send
                else
                    rvfi_steps <= 0;                                 // reset the step counter

                if (rvfi_scheck_cnt >= scheck_period)                // if count == period  we will do a state compare too
                    rvfi_scheck_cnt <= '0;                           // clear counter
                else
                    rvfi_scheck_cnt <= rvfi_scheck_cnt + valid_cnt;          

            end 
        end      
    end

    //-----------------------------------------------------------------------------------------------------------------
    // Periodic COMPARE REGISTER STATE logic:
    //
    //  COSIM RTL:
    //  - A compare_state is issued under 2 conditions:
    //        - We have executed N or more instructions
    //        - EOT has been detected
    //  - This will trigger 2 events 
    //        1) DPI call to send current state of GP/FP/VC register (fp and vc only if they have changed)  
    //              because this ifc is later in YML it will get received AFTER the RVFI packets are processed
    //        2) DPI call to end  RVFI packet via poke_events with the following flags:
    //              - force_steps:  only used during EOT to force the ISS to execute its remaining steps 
    //
    //  BRIDGE C++
    //  - FIRST retirement packet will execute the missing "steps" ... the other pkts will ignore this flag 
    //      - if "force_steps" flag is set to 1, then return back to rtl as there no instructions to process
    //  - GP/FP/VC register state messages received AFTER the RVFI (if it is sent)
    //       - compares registers  
    //-----------------------------------------------------------------------------------------------------------------

    assign compare_state = (rvfi_valid & ((rvfi_scheck_cnt >= scheck_period) & (scheck_period != 0))) | eot_found;
 

    // m_csri
    logic [CSR_COUNT-1:0] m_csris_valid;
    logic [CSR_COUNT-1:0] valid_d0;
    logic [CSR_COUNT-1:0] valid_d1;
    logic [CSR_COUNT-1:0][CSRLEN-1:0] addr_d1;
    logic [CSR_COUNT-1:0][63:0] data_d1;
    logic [CSR_COUNT-1:0][63:0] mask_d1;

    always @(posedge clk) begin
      for (int n = 0; n < CSR_COUNT; n++) begin
        valid_d1[n] <= csri[n].valid;
        addr_d1[n] <= csri[n].addr;
        data_d1[n] <= csri[n].data;
        mask_d1[n] <= csri[n].mask;
      end
    end

    bit [MAXCSR:0][CSR_SBITS-1:0] csr_sel;

    assign csr_sel = retsel(m_csris_valid);

    //-------------------------------------------------------------------------------------------
    // if csr_sel[MAXCSR] == 1 then we went 1 past the MAX number of DPI calls we can make
    //    only csr_sel[MAXCSR-1:0] are valid.
    //-------------------------------------------------------------------------------------------
    always @(posedge clk) begin
        assert (csr_sel[MAXCSR] == '1) else $error("More than %d CSR valids == 1",MAXCSR-1);
    end


    for (genvar n = 0; n < CSR_COUNT; n++) begin
        assign m_csris_valid[n] = rvfi_enabled & ~dut_reset & ((csri[n].valid & ~valid_d1[n]) | (csri[n].valid & ((csri[n].data !== data_d1[n]) | (csri[n].mask !== mask_d1[n]))));
    end
    for (genvar n = 0; n < MAXCSR; n++) begin
        assign m_csris[n].valid         = (csr_sel[n] != '1) ? 1'b1 : 1'b0; 
        assign m_csris[n].data.location = location;
        assign m_csris[n].data.cycle    = clocks;
        assign m_csris[n].data.hart     = NUM;
        assign m_csris[n].data.addr     = (csr_sel[n] != '1) ? csri[csr_sel[n]].addr : '0;
        assign m_csris[n].data.mask     = (csr_sel[n] != '1) ? csri[csr_sel[n]].mask : '0;
        assign m_csris[n].data.data     = (csr_sel[n] != '1) ? csri[csr_sel[n]].data : '0;
    end

    // m_mcmi_read
    for (genvar n = 0; n < NREAD; n++) begin
        assign m_mcmi_reads[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_read[n].valid;
        assign m_mcmi_reads[n].data.location = location;
        /* verilator lint_off WIDTH */
        assign m_mcmi_reads[n].data.cycle = mcmi_read[n].valid ? clocks : '0;
        /* verilator lint_on WIDTH */
        assign m_mcmi_reads[n].data.hart = NUM;
        assign m_mcmi_reads[n].data.order = mcmi_read[n].order;
        assign m_mcmi_reads[n].data.addr = mcmi_read[n].addr;
        assign m_mcmi_reads[n].data.mask = mcmi_read[n].mask;
        assign m_mcmi_reads[n].data.data = mcmi_read[n].data[63:0];
        assign m_mcmi_reads[n].data.amo = mcmi_read[n].amo;
        assign m_mcmi_reads[n].data.amo_op = mcmi_read[n].amo_op;
    end

    // m_mcmi_insert
    for (genvar n = 0; n < NINSERT; n++) begin
        assign m_mcmi_inserts[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_insert[n].valid;
        assign m_mcmi_inserts[n].data.location = location;
        assign m_mcmi_inserts[n].data.cycle = mcmi_insert[n].valid ? clocks : '0;
        assign m_mcmi_inserts[n].data.hart = NUM;
        assign m_mcmi_inserts[n].data.order = mcmi_insert[n].order;
        assign m_mcmi_inserts[n].data.addr = mcmi_insert[n].addr;
        assign m_mcmi_inserts[n].data.mask = mcmi_insert[n].mask;
        assign m_mcmi_inserts[n].data.data = mcmi_insert[n].data[63:0];
    end

    // m_mcmi_write
    for (genvar n = 0; n < NWRITE; n++) begin
        assign m_mcmi_writes[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_write[n].valid;
        assign m_mcmi_writes[n].data.location = location;
        assign m_mcmi_writes[n].data.cycle = mcmi_write[n].valid ? clocks : '0;
        assign m_mcmi_writes[n].data.hart = NUM;
        assign m_mcmi_writes[n].data.addr = mcmi_write[n].addr;
        assign m_mcmi_writes[n].data.mask = mcmi_write[n].mask;
        assign m_mcmi_writes[n].data.data = mcmi_write[n].data;

        assign mcmi_write_pokes[n] = (mcmi_write[n].addr <= scheck_addr_hi) & (mcmi_write[n].addr >= scheck_addr_lo) & mcmi_write[n].valid;

        //------------------------------------------------------------------------------------------- 
        // End-Of-Test logic:  memory write to designated address 
        //    - will cause a save-state event (force-steps=1 if NO instrs being retired currently
        //------------------------------------------------------------------------------------------- 
        assign eot_writes_found[n] = ((eot_addr != '0) &  mcmi_write[n].valid & (mcmi_write[n].addr == eot_addr) & ( mcmi_write[n].data[0] == 1'b1) & (mcmi_write[n].data[63:56] == 0)) ? 1'b1 : 1'b0;
    end

    assign mcmi_write_poke = (mcmi_write_pokes != '0);

    // m_mcmi_bypass
    for (genvar n = 0; n < NBYPASS; n++) begin
        assign m_mcmi_bypasss[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_bypass[n].valid;
        assign m_mcmi_bypasss[n].data.location = location;
        assign m_mcmi_bypasss[n].data.cycle = mcmi_bypass[n].valid ? clocks : '0;
        assign m_mcmi_bypasss[n].data.hart = NUM;
        assign m_mcmi_bypasss[n].data.order = mcmi_bypass[n].order;
        assign m_mcmi_bypasss[n].data.addr = mcmi_bypass[n].addr;
        assign m_mcmi_bypasss[n].data.mask = mcmi_bypass[n].mask;
        assign m_mcmi_bypasss[n].data.data = mcmi_bypass[n].data[63:0];
        assign m_mcmi_bypasss[n].data.amo = mcmi_bypass[n].amo;
        assign m_mcmi_bypasss[n].data.amo_op = mcmi_bypass[n].amo_op;
        //------------------------------------------------------------------------------------------- 
        // End-Of-Test logic:  memory write to designated address 
        //    - will cause a save-state event (force-steps=1 if NO instrs being retired currently
        //------------------------------------------------------------------------------------------- 
        assign eot_bypass_found[n] = ((eot_addr != '0) &  mcmi_bypass[n].valid & (mcmi_bypass[n].addr == eot_addr) & ( mcmi_bypass[n].data[0] == 1'b1) & (mcmi_bypass[n].data[63:56] == 0)) ? 1'b1 : 1'b0;
        assign mcmi_bypass_pokes[n] = (mcmi_bypass[n].addr <= scheck_addr_hi) & (mcmi_bypass[n].addr >= scheck_addr_lo) & mcmi_write[n].valid;
    end

    assign mcmi_bypass_poke = (mcmi_bypass_pokes != '0);

    assign eot_found = ((eot_writes_found != 0) | (eot_bypass_found != 0) | eot_max_instr) ? 1'b1 : 1'b0; 

    // m_mcmi_ifetch
    for (genvar n = 0; n < NIFETCH; n++) begin
        assign m_mcmi_ifetch_reqs[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_ifetch_req[n].valid;
        assign m_mcmi_ifetch_reqs[n].data.location = location;
        assign m_mcmi_ifetch_reqs[n].data.cycle = mcmi_ifetch_req[n].valid ? clocks : '0;
        assign m_mcmi_ifetch_reqs[n].data.hart = NUM;
        assign m_mcmi_ifetch_reqs[n].data.order = mcmi_ifetch_req[n].order;
        assign m_mcmi_ifetch_reqs[n].data.addr = mcmi_ifetch_req[n].addr;
    end

    for (genvar n = 0; n < NIFETCH; n++) begin
        assign m_mcmi_ifetch_resps[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_ifetch_resp[n].valid;
        assign m_mcmi_ifetch_resps[n].data.location = location;
        assign m_mcmi_ifetch_resps[n].data.cycle = mcmi_ifetch_resp[n].valid ? clocks : '0;
        assign m_mcmi_ifetch_resps[n].data.hart = NUM;
        assign m_mcmi_ifetch_resps[n].data.order = mcmi_ifetch_resp[n].order;
    end

    // m_mcmi_ievict
    for (genvar n = 0; n < NIEVICT; n++) begin
        assign m_mcmi_ievicts[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_ievict[n].valid;
        assign m_mcmi_ievicts[n].data.location = location;
        assign m_mcmi_ievicts[n].data.cycle = mcmi_ievict[n].valid ? clocks : '0;
        assign m_mcmi_ievicts[n].data.hart = NUM;
        assign m_mcmi_ievicts[n].data.addr = mcmi_ievict[n].addr;
    end

    // m_trap
    for (genvar n = 0; n < NRET; n++) begin
        assign m_traps[n].valid = RVFI_EN & rvfi_enabled & ~dut_reset & (rvfi[n].cause != 0);
        assign m_traps[n].data.location = location;
        assign m_traps[n].data.cycle = clocks;
        assign m_traps[n].data.cause = rvfi[n].cause;
    end

    // When using periodic whisper updates... check for eot if max instruction method is used
    assign eot_max_instr = ((scheck_period > 0) & (max_instructions > 0) &  ((instruction_cnt+valid_cnt) >= (max_instructions))) ? 1'b1: 1'b0;

    // m_debug
    logic debug_mode_d1;
    always @(posedge clk) begin
      debug_mode_d1 <= debug_mode;
    end
    assign m_debugs[0].valid = ~dut_reset & ((debug_mode & ~debug_mode_d1) | (~debug_mode & debug_mode_d1)) & rvfi_enabled;
    assign m_debugs[0].data.location = location;
    assign m_debugs[0].data.cycle = clocks;
    assign m_debugs[0].data.enter = debug_mode;
    assign m_debugs[0].data.exit = ~debug_mode;

    // m_core_intr
    rv_tester_pkg::interrupt_t wired_interrupt_d1;
    always @(posedge clk) begin
      if (reset) begin
        wired_interrupt_d1 <= 0;
      end else begin
        wired_interrupt_d1 <= wired_interrupt;
      end
    end
    assign m_core_intrs[0].valid = ~dut_reset & (|(wired_interrupt & ~wired_interrupt_d1) | |(~wired_interrupt & wired_interrupt_d1)) & rvfi_enabled;
    assign m_core_intrs[0].data.location = location;
    assign m_core_intrs[0].data.cycle = clocks;
    assign m_core_intrs[0].data.mip = get_mip(wired_interrupt);
    assign m_core_intrs[0].data.mip_mask = get_mip_mask(wired_interrupt, wired_interrupt_d1);
    assign m_core_intrs[0].data.mip_assert = get_mip_assert(wired_interrupt, wired_interrupt_d1);

    function automatic bit [63:0] get_mip(rv_tester_pkg::interrupt_t intr);
      bit [63:0] mip = 'h0;
      mip[13] = intr.lcofi;
      mip[12] = intr.sgei;
      mip[11] = intr.mei;
      mip[10] = intr.vsei;
      mip[9]  = intr.sei;
      mip[7]  = intr.mti;
      mip[6]  = intr.vsti;
      mip[5]  = intr.sti;
      mip[3]  = intr.msi;
      mip[1]  = intr.ssi;
      return mip;
    endfunction

    localparam imsic_whisper_delays = 5;
    rv_tester_params::mst_req_top imsic_interrupt_delays[imsic_whisper_delays:0];
    rv_tester_params::mst_req_top imsic_interrupt_delayed;
    assign imsic_interrupt_delays[0]=imsic_interrupt;
    genvar i;
    generate
      for (i=1; i <= imsic_whisper_delays; i=i+1) begin
        always @(posedge clk)
        imsic_interrupt_delays[i] <= imsic_interrupt_delays[i-1];
      end
    endgenerate
    assign imsic_interrupt_delayed = imsic_interrupt_delays[imsic_whisper_delays];

    // m_imsic_msi
    enum logic {idle, aw} msi_slave_state,msi_slave_state_d;
    logic msi_addr_in_imsic_range;
    always @(posedge clk) begin
       if (reset) begin
        msi_slave_state <= idle;
       end else begin
        msi_slave_state <= msi_slave_state_d;
       end
    end
    assign msi_slave_state_d = imsic_interrupt_delayed.w_valid ? idle : imsic_interrupt_delayed.aw_valid ? aw : msi_slave_state;
    assign msi_addr_in_imsic_range = (imsic_interrupt_delayed.aw.addr[31:0] >= 32'h40000000 &&  imsic_interrupt_delayed.aw.addr[31:0] < 32'h42000000) || (imsic_interrupt_delayed.aw.addr[31:0] >= 32'h44000000 &&  imsic_interrupt_delayed.aw.addr[31:0] < 32'h46000000);
    assign m_imsic_msis[0].valid = ~dut_reset & ( (msi_slave_state==aw | imsic_interrupt_delayed.aw_valid) & imsic_interrupt_delayed.w_valid & imsic_interrupt_delayed.w.strb=='hf & msi_addr_in_imsic_range) & rvfi_enabled;
    assign m_imsic_msis[0].data.location = location;
    assign m_imsic_msis[0].data.cycle = clocks;
    /* verilator lint_off WIDTH */
    assign m_imsic_msis[0].data.addr = imsic_interrupt_delayed.aw.addr;
    assign m_imsic_msis[0].data.data = imsic_interrupt_delayed.w.data;
    /* verilator lint_on WIDTH */

    function automatic bit [63:0] get_mip_mask(rv_tester_pkg::interrupt_t intr, rv_tester_pkg::interrupt_t intr_d1);
      bit [63:0] mask = 'h0;
      mask[13] = (intr.lcofi & ~intr_d1.lcofi);
      mask[12] = (intr.sgei & ~intr_d1.sgei) | (~intr.sgei & intr_d1.sgei);
      mask[11] = (intr.mei & ~intr_d1.mei) | (~intr.mei & intr_d1.mei);
      mask[10] = (intr.vsei & ~intr_d1.vsei) | (~intr.vsei & intr_d1.vsei);
      mask[9]  = (intr.sei & ~intr_d1.sei) | (~intr.sei & intr_d1.sei);
      mask[7]  = (intr.mti & ~intr_d1.mti) | (~intr.mti & intr_d1.mti);
      mask[6]  = (intr.vsti & ~intr_d1.vsti) | (~intr.vsti & intr_d1.vsti);
      mask[5]  = (intr.sti & ~intr_d1.sti) | (~intr.sti & intr_d1.sti);
      mask[3]  = (intr.msi & ~intr_d1.msi) | (~intr.msi & intr_d1.msi);
      mask[1]  = (intr.ssi & ~intr_d1.ssi) | (~intr.ssi & intr_d1.ssi);
      return mask;
    endfunction

    function automatic bit [63:0] get_mip_assert(rv_tester_pkg::interrupt_t intr, rv_tester_pkg::interrupt_t intr_d1);
      bit [63:0] mask = 'h0;
      mask[13] = (intr.lcofi & ~intr_d1.lcofi);
      mask[12] = (intr.sgei & ~intr_d1.sgei);
      mask[11] = (intr.mei & ~intr_d1.mei);
      mask[10] = (intr.vsei & ~intr_d1.vsei);
      mask[9]  = (intr.sei & ~intr_d1.sei);
      mask[7]  = (intr.mti & ~intr_d1.mti);
      mask[6]  = (intr.vsti & ~intr_d1.vsti);
      mask[5]  = (intr.sti & ~intr_d1.sti);
      mask[3]  = (intr.msi & ~intr_d1.msi);
      mask[1]  = (intr.ssi & ~intr_d1.ssi);
      return mask;
    endfunction


    always @(posedge tb_clk) begin
      if (reset) begin
        /* verilator lint_off BLKSEQ */
        max_cycle = cvm_plusargs::get_ulongint("max_cycle");
        max_stall_cycle = cvm_plusargs::get_int("max_stall_cycle");
        scheck_period = cvm_plusargs::get_int("scheck_period");
        scheck_addr_hi = cvm_plusargs::get_ulongint("scheck_addr_hi");
        scheck_addr_lo = cvm_plusargs::get_ulongint("scheck_addr_lo");
        max_instructions = cvm_plusargs::get_ulongint("max_instr");
        hart_enable_mask = cvm_plusargs::get_ulongint("hart_enable_mask");
        /* verilator lint_on BLKSEQ */
        cycles_since_retire <= 0;
        boot_wfi <= '0;
        instruction_cnt <= '0;
      end else if(!dut_reset) begin
        cycles_since_retire <= cycles_since_retire + 1;
        if (rvfi_valids != '0) begin
            instruction_cnt <= instruction_cnt + valid_cnt;
        end
        if (rvfi[0].valid !== 0) begin
          cycles_since_retire <= 0;
        end
        if (NUM != 0 && hart_enable_mask[NUM] == 0 && rvfi[0].valid !== 0 && rvfi[0].insn[6:0] == 7'h73 && rvfi[0].pc_rdata < 'h20000) begin // WFI
          boot_wfi <= '1;
        end
        if (max_stall_cycle > 0 && cycles_since_retire > max_stall_cycle && !boot_wfi) begin
          $display("Error: Hart %0d: No instruction retired for max_stall_cycle (%0d) cycles", NUM, max_stall_cycle);
          cosim_terminate();
        end
        if (max_cycle > 0 && clocks > max_cycle) begin
          $display("Error: Hart %0d:  Test running for max_cycle (%0d) cycles - stuck in a loop, or too long", NUM, max_cycle);
          cosim_terminate();
        end
      end
    end

endmodule
