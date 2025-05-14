//==============================================================================================================================
// FEATURES ADDED:
// --------------
//
//    PERIODIC-STATE_CHECK :   reduces RVFI msg traffic to C by sending only what is needed
//
//      ENABLING:
//          - +cosim_period=<n>  where <n> > 0
//          - +debug_cycle=<n>   where <n> > 0   -- turns ON extra C++ messages to aid in debug
//          - +debug_high=<n>    where <n> > 0   -- DISABLES PSC mode when clock count >= debug_low and <= debug_high and debug_high>0 
//          - +debug_low=<n>     where <n> > 0   -- See debug_high (this is useful in debugging why whisper is miscomparing) 
//
//      USAGE
//          - sends RVFI packets only when necessary to keep COSIM model in-sync.
//                - packets that require COSIM to be updated are refered to as 'pokes'
//                - Poke events are:
//                     - CSR read/writes
//                     - SC read/writes  
//                     - INTERRUPT memory writes  
//                     - Exceptions 
//                     - PC==debug_entry_pc or PC==debug_exit_c (or ANY instruction in between) 
//                     - if any of the above cause a POKE.. but last_uop=0...then POKE again when last_uop=1 for that 'order' 
//          - sends STEP packets to push the COSIM model along when needed:
//                - when an RVFI has to be sent and step count > 0 
//                - when instruction orders 'skip' values
//          - sends GP/FP/VP registers for comparison       
//                - sent when # of retired instructions > 'cosim_period'  AND
//                - when there are no retiring instructions with last_uop=0  AND
//                - a register value has been written since the last update 
//
//      ERROR CONDITIONS:
//          - Feature cannot be used with any mode requiring every instruction to be sent
//                - emulate_debug_mode=1
//                - cosim_resynch=1
//                - mcm=1   (not yet validated for MCM testing) 
//           
//==============================================================================================================================
module cosim_reg_bank
#(
  parameter int NUM    = 1,
  parameter int WIDTH  = 1,
  parameter int PORTS  = 1,

  localparam int AW = $clog2(NUM)
)(
  input  logic clk,
  input  logic reset,
  input  logic reset_changed,
  input  logic             wen   [PORTS],
  input  logic [WIDTH-1:0] wdata [PORTS],
  input  logic [AW     :0] waddr [PORTS],
  output logic [NUM-1:0][WIDTH-1:0] regs,
  output logic             changed
);

  logic [NUM-1:0][WIDTH-1:0] regs_1T;

  always_comb begin
    regs = regs_1T;
    for (int i = 0; i < PORTS; i++) begin
      if (wen[i] & ~waddr[i][AW]) begin
        regs[waddr[i]] = wdata[i];
      end
    end
  end

  // FIXME remove reset and send an enable vec for what's been written
  always_ff @(posedge clk) begin
    if (reset) begin
      for (int i = 0; i < NUM; i++) begin
        regs_1T[i] <= '0;
      end
    end else begin
      regs_1T <= regs;
    end
  end

  logic changed_1T;

  always_comb begin
    changed = changed_1T;
    if (reset || reset_changed) begin
      changed = 1'b0;
    end else if (!changed) begin
      for (int i = 0; i <PORTS; i++) begin
        changed |= wen[i];
      end
    end
  end

  always_ff @(posedge clk) begin
    changed_1T <= changed;
  end

endmodule

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
    parameter int MAX_CSR_AFTER_NRET = 3,
    parameter type rule_t = axi_pkg::xbar_rule_64_t,
    parameter int unsigned NoAddrRules = 20,
    `TOPOLOGY,
    `RV_TESTER_TRANSACTIONS_COSIM_OUTPUT_PARAMS
)(
    input tb_clk,
    input clk,
    input reset,
    input dut_reset,
    input logic [63:0] clocks,
    input rule_t [NoAddrRules-1:0] addr_map,
    input rvfi_t [NRET-1:0] rvfi,
    input csri_t csri,
    input mcmi_t [NREAD-1:0] mcmi_read,
    input mcmi_t [NINSERT-1:0] mcmi_insert,
    input mcmi_t [NWRITE-1:0] mcmi_write,
    input mcmi_t [NBYPASS-1:0] mcmi_bypass,
    input mcmi_t [NIFETCH-1:0] mcmi_ifetch_req,
    input mcmi_t [NIFETCH-1:0] mcmi_ifetch_resp,
    input mcmi_t [NIEVICT-1:0] mcmi_ievict,
    input rv_tester_pkg::nmi_t nmi_pend,
    input rv_tester_params::interrupt_pend_t interrupt_pend,
    input logic [63:0] mtime,
    input rv_tester_params::msi_t imsic_msi,
    input debug_mode,
    input haltreq,
    input longint eot_addr,
    input bit poke_event_in,
    output bit poke_event_out,
    output rv_tester_pkg::terminate_t terminate,
    input logic disable_checks,
    output logic boot_done,
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
bit [PA_WIDTH-1:0] debug_entry_pc_const='h0a110800;
bit [PA_WIDTH-1:0] debug_exit_pc_const='h0a110860;
bit [PA_WIDTH-1:0] mmr_hi_addr_const='h42a1FFFF;
bit [PA_WIDTH-1:0] mmr_lo_addr_const='h42000000;

genvar gi,gj;

localparam CDEPTH = 64;
localparam CWIDTH = 16;
localparam CIBITS = $clog2(CDEPTH);
localparam CTBITS = 64-CIBITS;

localparam CAM_DEPTH = 64;
localparam CAM_WIDTH = 16;
localparam CAM_IBITS = $clog2(CAM_DEPTH);
localparam CAM_ILBIT = 0;
localparam CAM_IHBIT = CAM_IBITS;


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

    function automatic bit [CSR_COUNT-1:0] get_csr_mask();
      bit [CSR_COUNT-1:0] mask = '1;
      mask[MIP]             = 0; //sepearate AIA flow takes care of mip update
      mask[TIME]            = 0; //time csr gets updated evey clock cycle
      mask[MCYCLE]          = 0; //mcycle csr gets updated evey clock cycle
      mask[MINSTRET]        = 0; //whisper models this instruction retire counter
      mask[CXTVALSPEC]      = 0; // fe,mc,ls holds copy of same and are not updated across
      mask[CXINSTSPEC]      = 0; //-------------------""----------------------------------
      mask[CMCTHRCFG0]      = 0; //thermal throttle csr not important fucntionally
      mask[VSTOPEI]         = 0; //interrupt csr update are handled separately
      mask[MHPMCOUNTER3]    = 0; //perf counter 
      mask[MHPMCOUNTER4]    = 0; //perf counter 
      mask[MHPMCOUNTER5]    = 0; //perf counter 
      mask[MHPMCOUNTER6]    = 0; //perf counter 
      mask[MHPMCOUNTER7]    = 0; //perf counter 
      mask[MHPMCOUNTER8]    = 0; //perf counter 
      mask[MHPMCOUNTER9]    = 0; //perf counter 
      mask[MHPMCOUNTER10]   = 0; //perf counter 
      return mask;
    endfunction

    //---------------------------------------------------------------
    // Function to return the count of VALIDS with UNIQUE ORDER bits
    //---------------------------------------------------------------
    function automatic [$clog2(NRET+1)-1:0] valid_count(input bit [NRET-1:0] valid, input bit [NRET-1:0][63:0] order, input bit [NRET-1:0] last_uops,num);
        bit [63:0] corder=0;
        valid_count = 0;
        /* verilator lint_off WIDTH */
        for(int i=0;i<num;i=i+1) begin
            if ((valid[i] == 1'b1) & (last_uops[i] == 1'b1)) begin
               valid_count = valid_count + 1;
            end
        end
        /* verilator lint_on WIDTH */
    endfunction

    //----------------------------------------------------------
    // Function to return the highest valid order bits
    //----------------------------------------------------------
    function automatic [63:0] max_order(input bit [NRET-1:0] valid, input bit [NRET-1:0][63:0] order);
        max_order = 0;
        /* verilator lint_off WIDTH */
        for(int i=NRET-1;i>=0;i=i-1) begin
            if (valid[i] == 1) begin
               return(order[i]);
            end
        end
        /* verilator lint_on WIDTH */
    endfunction

    //----------------------------------------------------------------------------
    // function memmap_decode: compares address to memory map ranges 
    //----------------------------------------------------------------------------
    
    function automatic bit memmap_decode(input rule_t [NoAddrRules-1:0] mem_map, input bit [PA_WIDTH-1:0] address );
        memmap_decode = 1'b0;
        if ((address < mmr_hi_addr_const) & (address >= mmr_lo_addr_const))
           return(1'b1);
        /* verilator lint_off WIDTH */
        for(int i=0;i<NoAddrRules;i=i+1) begin
            if ((address >= mem_map[i].start_addr) & (address < mem_map[i].end_addr) & (mem_map[i].idx == 1)) begin
                return(1'b1);
            end
        end
        /* verilator lint_on WIDTH */
    endfunction

    import "DPI-C" context function void cosim_set_scope(int unsigned location);
    import "DPI-C" context function int is_eot_tohost();
    //import "DPI-C" context function void eot_hw_process(longint unsigned hart, longint unsigned cycles, longint unsigned addr, longint unsigned data);
    //import "DPI-C" context function void call_check_max_instr(longint unsigned cycles, longint unsigned instr_count);


    bit PSC_enabled;
    typedef longint unsigned LU;
    parameter int unsigned location = cvm_topology_gen::get_location (topology.TOP.PLATFORM.COSIM.ID, NUM);
    bit rvfi_enabled,mcm_enabled,offline_dpi;
    bit offline_dpi_test;                          // this disables the sending of mcmi_bypass and mcmi_insert even when to_host == 1
    bit poke_mip_timer;

    //int mcm_value;
    longint unsigned psc_off_low  = 0;
    longint unsigned psc_off_high = 0;
    bit to_host;
    int unsigned cosim_period=0;
    int unsigned PSC_period=0;

    bit get_cosim_compare_values = 1;
    bit reset_d1 = 1;
    bit [5:0]           rd_addr     [NRET    ]; //register-retire load enable
    bit [5:0]           frd_addr    [NRET    ]; //register-retire load enable
    bit [5:0]           vrd_addr    [NRET    ]; //register-retire load enable
    bit                 rd_load     [NGP_REGS]; //register-retire load enable
    bit                 frd_load    [NFP_REGS];
    bit                 vrd_load    [NVC_REGS];
    bit [GP_WIDTH-1:0]  rd_wdata    [NRET    ];
    bit [NGP_REGS-1:0][GP_WIDTH-1:0]  gp_wdata_in;
    bit [FP_WIDTH-1:0]  frd_wdata   [NRET    ];
    bit [NFP_REGS-1:0][FP_WIDTH-1:0]  fp_wdata_in;
    bit [VC_WIDTH-1:0]  vrd_wdata   [NRET    ];
    bit [NVC_REGS-1:0][VC_WIDTH-1:0]  vc_wdata_in;
    bit                 gp_reg_written;
    bit                 fp_reg_written;
    bit                 vc_reg_written;

    bit               poke_debug_event;
    bit [NWRITE-1:0]  mcmi_write_pokes;
    bit [NBYPASS-1:0] mcmi_bypass_pokes;
    bit [NREAD-1:0]   mcmi_read_pokes;
    bit [NINSERT-1:0] mcmi_insert_pokes;
    bit [NIEVICT-1:0] mcmi_ievict_pokes;


    bit csrrw_valid;
    bit scrw_valid ;
    bit devrd_valid;
    bit mflag_valid;
    bit gpwa5_valid;

    int unsigned           cpu_id;
    longint unsigned       mintr_cnt;
    longint unsigned       csrrw_cnt;
    longint unsigned       scrw_cnt ;
    longint unsigned       devrd_cnt;
    longint unsigned       mflag_cnt;
    longint unsigned       gpwa5_cnt;
    longint unsigned       rvfi_cnt;
    longint unsigned       mrvfi_cnt;
    longint unsigned       mcm_cnt;

    bit               force_compare;


    bit [31:0]             rvfi_scheck_cnt;
    bit [NRET-1:0]         rvfi_valids;
    bit [NRET-1:0][63:0]   rvfi_orders;
    bit [NRET-1:0]         rvfi_luops;
    bit [NRET-1:0]         rvfi_val_luops;
    bit [NRET-1:0]         rvfi_val_luops_d1;
    bit [NRET-1:0][11:0]   rvfi_csr_addr;

    bit                    rvfi_multi_uop;
    bit [63:0]             rvmax_order;
    bit [63:0]             rvfi_steps;
    bit [63:0]             rvfi_step_cnt;
    bit [63:0]             rvfi_skips;
    bit [63:0]             rvfi_exp_order;
    bit                    rvfi_skip;
    bit                    order_skip;
    bit                    send_steps;
    bit                    rvfi_first_valid;
    bit [63:0]             val0_order;
    bit                    rvgp_valids[NRET];
    bit [NRET-1:0]         gp_loadn[NGP_REGS-1:0];
    bit                    gp_load[NGP_REGS-1:0];
    bit                    rvfp_valids[NRET];
    bit                    rvvc_valids[NRET];
    bit [NRET-1:0]         sc_rw;                           // instr= sc..  poke
    bit [NRET-1:0]         csr_rw;                          // instr= csr..  poke
    bit [NRET-1:0]         msret;                           // instr= mret or sret  poke
    bit [NRET-1:0]         fence;
    bit [NRET-1:0]         rvfi_excps;
    bit [NRET-1:0]         vec_crack;                       // cracked vector operation
    bit [NRET-1:0]         mem_write;                       // poke all memory writes IF mcm enabled 
    bit [NRET-1:0]         mem_read;                        // poke all memory read IF  mcm enabled
    bit [NRET-1:0]         intr_memw;
    bit [NRET-1:0]         cmp_memw;
    bit [NRET-1:0]         gp_waddr5;
    bit [NRET-1:0]         fp_waddr5;
    bit [NRET-1:0]         vc_waddr5;
    bit [NRET-1:0]         mflags;
    bit                    reg_waddr5_event;
    bit                    rvfi_excp_ip;                    // rvfi exeption packets beging processed
    bit                    rvfi_dbg_excp;
    bit [NRET-1:0]         poke_events;                     // events that should cause a Poke in Whisper 
    bit [NRET-1:0]         enter_dbg;                       // event when PC == debug_entry_pc
    bit [NRET-1:0]         exit_dbg;                        // event when PC == debug_exit_pc
    bit [NRET-1:0]         debug_read;                      // event when reading memory in debug_entry_pc and debug_exit_pc range
    bit [NRET-1:0]         device_read;                     // event memory read to a device in our address map 
    bit [NRET-1:0]         poke_no_uop;                     // events that should cause a Poke in Whisper 
    bit                    mintr;                           // event when m_trap senns an interrupt/exception
    bit                    poke_interrupt;                  // event when m_trap senns an interrupt/exception
    bit                    mtrap_valid;                     // event when m_trap senns an interrupt/exception
    bit                    imsic_valid;                     // event when m_trap senns an interrupt/exception
    bit                    rvfi_debug_mode;
    bit                    rvfi_debug_mode_s;
    bit [NRET-1:0][63:0]      rvfi_last_uop_orders;        // Tracks orders from previus cycle that "poked" but had last_uop=0
    bit [NRET-1:0][NRET-1:0]  rvfi_last_uop_events;           // matchs current order to previous last_poke_orders that last_uop=1 now
    bit                    poke_last_uop_event;
    bit [NRET-1:0][63:0]      rvfi_last_insn_orders;        // Tracks orders from previus cycle that "poked" but had last_uop=0
    bit [NRET-1:0][NRET-1:0]  rvfi_last_insn_events;           // matchs current order to previous last_poke_orders that last_uop=1 now
    bit                    poke_last_insn_event;
    bit [NWRITE-1:0]       eot_write_found;                // end-of-test event found in mcmi_writes ifc
    bit [NWRITE-1:0][63:0] eot_write_data;                // end-of-test event found in mcmi_bypass ifc
    bit [NBYPASS-1:0]      eot_bypass_found;                // end-of-test event found in mcmi_bypass ifc
    bit [NBYPASS-1:0][63:0]  eot_bypass_data;                // end-of-test event found in mcmi_bypass ifc
    bit [NINSERT-1:0]      eot_insert_found;                // end-of-test event found in mcmi_insert ifc
    bit [NINSERT-1:0][63:0]  eot_insert_data;                // end-of-test event found in mcmi_insert ifc
    longint unsigned       mcmi_write_addr[NWRITE-1:0]; 
    longint unsigned       mcmi_write_data[NWRITE-1:0];  
    longint unsigned       mcmi_insert_addr[NINSERT-1:0];  
    longint unsigned       mci_insert_data[NINSERT-1:0];   
    longint unsigned       mcmi_bypass_addr[NBYPASS-1:0];    
    longint unsigned       mcmi_bypass_data[NBYPASS-1:0];     
    bit [46:0]             eot_write_fail;                 // end-of-test code mcm_writes
    bit [46:0]             eot_insert_fail;                // end-of-test code mcm_inserts
    bit [46:0]             eot_bypass_fail;                 // end-of-test code mcm_byapss
    bit                    eot_write_pass;                 // end-of-test code mcm_writes
    bit                    eot_insert_pass;                // end-of-test code mcm_inserts
    bit                    eot_bypass_pass;                 // end-of-test code mcm_byapss
    bit                    eot_exit_pass;                  
    bit [46:0]             eot_exit_fail;                  
    bit [$clog2(NRET+1)-1:0] valid_cnt;                     // number of instructioncs retired this clock 
    bit [$clog2(NRET+1)-1:0] valid_icnt[NRET-1:0];          // number of instructions retired up to this retire index 
    bit [63:0]             instr_icnt[NRET-1:0];          // number of instructions retired up to this retire index 
    bit [NRET-1:0]         instr_imax;

    bit                    eot_found;                       // end-of-test event found
    bit                    eot_found_d1;                       // end-of-test event found
    bit                    eot_max_instr;                   // max # instructions end-of-test event found
    bit                    rvfi_valid;
    bit                    send_rvfi;
    bit                    send_regs;
    bit                    gp_changed; 
    bit                    fp_changed; 
    bit                    vc_changed; 

    /* verilator lint_off UNOPTFLAT */
    bit [NRET-1:0]      rvfi_ucode;
    bit [NRET-1:0]      rvfi_last_uop;
    bit [NRET-1:0]      rvfi_priv_change;
    bit [NRET-1:0][3:0] rvfi_priv;
    bit [NRET-1:0]      rvfi_patch_mode;
    bit [NRET-1:0]      rvfi_set_patch;
    bit [NRET-1:0]      rvfi_clr_patch;
    /* verilator lint_on UNOPTFLAT */
    bit                 rvfi_ucode_S;
    bit                 rvfi_last_uop_S;
    bit                 rvfi_priv_change_S;
    bit [3:0]           rvfi_priv_S;
    bit                 rvfi_patch_mode_S;
    bit [NRET-1:0]      rvfi_instr_ucode;
    bit [NRET-1:0]      rvfi_first_uop;
    bit [NRET-1:0][3:0] rvfi_instr_priv;
    bit [NRET-1:0][3:0] rvfi_mode;
    bit                 rvfi_trap_patch;
    bit                 rvfi_trap_pmode;
    bit                 poke_patch_mode;

    bit [63:0]          eoti_data;

    // Timeout checks
    int max_stall_cycle = 50000;
    longint unsigned max_cycle;
    longint unsigned max_instructions;
    longint unsigned instruction_cnt;
    longint unsigned instr_count;
    bit [PA_WIDTH-1:0] debug_entry_pc;
    bit [PA_WIDTH-1:0] debug_exit_pc;
    longint unsigned debug_entry_pc_arg;
    longint unsigned debug_exit_pc_arg;
    int cycles_since_retire;
    int hart_enable_mask;
    int nharts;
    longint unsigned hart;
    bit boot_wfi;
    bit cosim_terminate_sent;

    assign cpu_id = NUM;

    //--------------------------------------------------------------------------------------------
    // Track writes to GP,FP,VEC registers for comparison with Whisper
    //--------------------------------------------------------------------------------------------

    cosim_reg_bank #(NGP_REGS, GP_WIDTH, NRET) gp_regs_bank (
      .clk          (clk),
      .reset        (reset),
      .wen          (rvgp_valids),
      .wdata        (rd_wdata),
      .waddr        (rd_addr),
      .regs         (gp_wdata_in),
      .changed      (gp_changed),
      .reset_changed(send_regs)
    );

    cosim_reg_bank #(NFP_REGS, FP_WIDTH, NRET) fp_regs_bank (
      .clk          (clk),
      .reset        (reset),
      .wen          (rvfp_valids),
      .wdata        (frd_wdata),
      .waddr        (frd_addr),
      .regs         (fp_wdata_in),
      .changed      (fp_changed),
      .reset_changed(send_regs)
    );

    cosim_reg_bank #(NVC_REGS, VC_WIDTH, NRET) vc_regs_bank (
      .clk          (clk),
      .reset        (reset),
      .wen          (rvvc_valids),
      .wdata        (vrd_wdata),
      .waddr        (vrd_addr),
      .regs         (vc_wdata_in),
      .changed      (vc_changed),
      .reset_changed(send_regs)
    );

    for(genvar n=0;n<NRET;n=n+1) begin
        assign rvfi_valids[n] = rvfi[n].valid ;
        assign rvfi_orders[n] = rvfi[n].order ;
        assign rvfi_luops[n]  = rvfi[n].last_uop;
        assign rvfi_val_luops[n]  = rvfi[n].valid & ~rvfi[n].last_uop;
        assign rvfi_csr_addr[n] = |rvfi[n].csr_wmask ? rvfi[n].csr_addr[11:0] : 12'h000;
        assign rd_addr[n]     = rvfi[n].rd_addr[5:0];
        assign frd_addr[n]    = rvfi[n].frd_addr[5:0];
        assign vrd_addr[n]    = rvfi[n].vrd_addr[5:0];
        assign rd_wdata[n]    = rvfi[n].rd_wdata;
        assign frd_wdata[n]   = rvfi[n].frd_wdata;
        assign vrd_wdata[n]   = rvfi[n].vrd_wdata;
        assign rvgp_valids[n] = rvfi[n].valid & (rvfi[n].rd_addr[4:0] != 0) ? 1'b1 : 1'b0;
        assign rvfp_valids[n] = rvfi[n].valid & rvfi[n].frd_valid;
        assign rvvc_valids[n] = rvfi[n].valid & rvfi[n].vrd_valid;

        for(genvar r=0;r<NGP_REGS;r=r+1) begin
           assign gp_loadn[r][n] = (rvfi[n].valid & (rvfi[n].rd_addr[4:0] == r) ? 1'b1 : 1'b0);
        end
    end
    for(genvar r=0;r<NGP_REGS;r=r+1) begin
        assign gp_load[r] = (gp_loadn[r][NRET-1:0] != '0) ? 1'b1 : 1'b0;
    end

    always @(posedge clk) begin
       if (reset) 
          rvfi_val_luops_d1 <= '0; 
       else
          rvfi_val_luops_d1 <= rvfi_val_luops;
    end

    assign rvfi_valid = | rvfi_valids;                        // OR of all valid retires

    //---------------------------------------------------------------------------------
    // POKE EVENTS logic
    //   - Several cases where we need to send the RVFI packet...
    //        1) SC instruction decoded
    //        2) CSR instruction decoded
    //        3) Execptions 
    //        4) Interrupts 
    //        5) Interrupts Causes 
    //        6) Multi-cycle UOPS (where last_uop == 0) 
    //
    //---------------------------------------------------------------------------------
    for(genvar n=0;n<NRET;n=n+1) begin
        assign sc_rw[n]        = rvfi[n].valid & ((rvfi[n].insn[6:0] == 7'b0101111) & (rvfi[n].insn[14:13] == 2'b01)  & (rvfi[n].insn[31:27] == 5'b00011));
        assign csr_rw[n]       = rvfi[n].valid & (((rvfi[n].insn[6:0] == 7'b1110011) & (rvfi[n].insn[14:12] != 3'b000)) | |rvfi[n].csr_wmask);
        assign msret[n]        = rvfi[n].valid & (rvfi[n].insn[31:30] == 2'b00) & (rvfi[n].insn[28:0] == 29'h10200073);    // mret or sret
        assign fence[n]        = rvfi[n].valid & ((rvfi[n].insn[31:0] & 32'hF000607F) == 32'h0000000F);
        assign intr_memw[n]    = rvfi[n].valid & (rvfi[n].mem_wmask != 0) & (rvfi[n].mem_wmask[1:0]==2'b11) & (rvfi[n].mem_paddr[PA_WIDTH-1:32] == '0) & ((rvfi[n].mem_paddr[31:25]==7'h20) | (rvfi[n].mem_paddr[31:25]==7'h22));
        //assign enter_dbg[n]    = rvfi[n].valid & (rvfi[n].pc_rdata == 64'(debug_entry_pc));
        assign enter_dbg[n]    = m_traps[0].valid & (
                                    (m_traps[0].data.id==rv_tester_pkg::INTR) |
                                    (m_traps[0].data.id==rv_tester_pkg::EXCP) & (m_traps[0].data.cause[7:0] ==33)
                                   );

        assign exit_dbg[n]     = rvfi[n].valid & (rvfi[n].pc_rdata == 64'(debug_exit_pc));
        assign device_read[n]  = rvfi[n].valid & (rvfi[n].mem_rmask != '0) & memmap_decode(addr_map, rvfi[n].mem_paddr);

        assign mflags[n]       = rvfi[n].flags_valid; 
        assign rvfi_excps[n]   = ~rvfi[n].cause[63] & (rvfi[n].cause != '0); 
        assign vec_crack[n]   = rvfi[n].valid & rvfi[n].vec & !rvfi[n].last_uop;
        assign mem_write[n]   = rvfi[n].valid & (rvfi[n].mem_wmask !=0) & mcm_enabled;
        assign mem_read[n]    = rvfi[n].valid & (rvfi[n].mem_rmask !=0) & mcm_enabled;

        assign gp_waddr5[n]    = rvgp_valids[n] & rd_addr[n][5];                  // Writing to a GP register above 31...poke
        assign fp_waddr5[n]    = rvfp_valids[n] & frd_addr[n][5];                 // Writing to a FP register above 31...poke
        assign vc_waddr5[n]    = rvvc_valids[n] & vrd_addr[n][5];                 // Writing to a VC register above 31...poke

        assign poke_events[n]  = sc_rw[n] | csr_rw[n] | intr_memw[n] | gp_waddr5[n] | poke_interrupt |  vec_crack[n] |
                                 device_read[n] | poke_patch_mode |  mem_write[n] | mem_read[n];

        //assign poke_events[n]  = sc_rw[n] | csr_rw[n] | intr_memw[n] | msret[n] | gp_waddr5[n] | mintr | mflags[n] |
        //                         enter_dbg[n] | exit_dbg[n] | debug_read[n] | device_read[n] | fence[n] ;
    end

    assign rvfi_multi_uop = ((rvfi_val_luops != '0) | (rvfi_val_luops_d1 != '0)) ? 1'b1 : 1'b0;

    //-----------------------------------------------------------------------------------
    // We do not send registers for comparison when there is an exception in debug_mode
    //-----------------------------------------------------------------------------------
    assign rvfi_dbg_excp  = ((rvfi_excps != '0)  | rvfi_excp_ip) & rvfi_debug_mode;

    //---------------------------------------------------------------------------------------------------------
    // writing a register > 31 is a special case where we poke AND send the registers
    //---------------------------------------------------------------------------------------------------------
    assign reg_waddr5_event = ((gp_waddr5 != '0) | (fp_waddr5 != '0) | (vc_waddr5 != '0)) ? 1'b1 : 1'b0;

    assign imsic_valid = m_imsic_msis[0].valid;
    assign mtrap_valid = m_traps[0].valid;
    assign csrrw_valid = (csr_rw != '0); 
    assign scrw_valid  = (sc_rw != '0); 
    assign gpwa5_valid = (gp_waddr5 != '0); 
    assign mflag_valid = (mflags != '0); 
    assign devrd_valid = (device_read != '0); 

    for(genvar n=0;n<NRET;n=n+1) begin
        always @(posedge clk)
        begin
            if (reset | (rvfi[n].last_uop & rvfi[n].valid)) begin
               rvfi_last_uop_orders[n] <= '0;
            end
            else begin  
               if (rvfi[n].valid & ~rvfi[n].last_uop & poke_event_out)
                  rvfi_last_uop_orders[n] <= rvfi[n].order;
            end

            if (reset | (rvfi[n].last_insn & rvfi[n].valid)) begin
               rvfi_last_insn_orders[n] <= '0;
            end
            else begin  
               if (rvfi[n].valid & ~rvfi[n].last_insn & poke_event_out)
                  rvfi_last_insn_orders[n] <= rvfi[n].order;
            end
        end
 
        
        
        //----------------------------------------------------------------------------------------------------------
        // Match current order with last clocks rvfi orders that had a "poke" but last_uop=0 .. now last_up==1
        //----------------------------------------------------------------------------------------------------------
        for(genvar m=0;m<NRET;m=m+1) begin
            //assign rvfi_last_uop_events[n][m] = rvfi[n].valid & (rvfi[n].order == rvfi_last_uop_orders[m]);
            assign rvfi_last_uop_events[n][m] = rvfi[n].valid & (rvfi[n].order == rvfi_last_uop_orders[m]) & rvfi[n].last_uop ;  
            assign rvfi_last_insn_events[n][m] = rvfi[n].valid & (rvfi[n].order == rvfi_last_insn_orders[m]) & rvfi[n].last_insn;  
        end

    end

    assign rvfi_debug_mode = rvfi_debug_mode_s | (enter_dbg != '0);

    always @(posedge clk)
    begin
        if (reset) 
           rvfi_debug_mode_s <= 1'b0;
        else
        if (enter_dbg != '0) 
           rvfi_debug_mode_s <= 1'b1;
        else
        if (exit_dbg != '0) 
           rvfi_debug_mode_s <= 1'b0;

        //---------------------------------------------------------------
        // monitor interrupt valids for poke events
        //---------------------------------------------------------------
        if (reset) begin
           rvfi_excp_ip <= 1'b0;
           mintr <= 1'b0;
           mintr_cnt <= '0;
           csrrw_cnt <= '0;
           scrw_cnt  <= '0;
           devrd_cnt <= '0;
           mflag_cnt <= '0;
           gpwa5_cnt <= '0;
           rvfi_cnt  <= '0;
           mrvfi_cnt <= '0;
           mcm_cnt   <= '0;
           eot_found_d1 <= 1'b0;
        end
        else begin
           if (rvfi_excps != '0) begin
              rvfi_excp_ip <=  1'b1; 
           end
           else begin
              if ((rvfi_val_luops == '0) & rvfi_valid) begin
                 rvfi_excp_ip <=  1'b0; 
              end
           end
           if (mtrap_valid | (m_interrupt_pends[0].valid) | imsic_valid) begin
              mintr <= 1'b1;
           end
           else begin
              if (rvfi_valid & poke_event_in) begin
                  mintr <= 1'b0;
              end
           end
           if ( mintr & ~csrrw_valid & ~scrw_valid & ~devrd_valid & ~mflag_valid & ~gpwa5_valid ) mintr_cnt <= mintr_cnt + 1; 
           if (~mintr &  csrrw_valid & ~scrw_valid & ~devrd_valid & ~mflag_valid & ~gpwa5_valid ) csrrw_cnt <= csrrw_cnt + 1; 
           if (~mintr & ~csrrw_valid &  scrw_valid & ~devrd_valid & ~mflag_valid & ~gpwa5_valid ) scrw_cnt  <= scrw_cnt + 1; 
           if (~mintr & ~csrrw_valid & ~scrw_valid &  devrd_valid & ~mflag_valid & ~gpwa5_valid ) devrd_cnt <= devrd_cnt + 1; 
           if (~mintr & ~csrrw_valid & ~scrw_valid & ~devrd_valid &  mflag_valid & ~gpwa5_valid ) mflag_cnt <= mflag_cnt + 1; 
           if (~mintr & ~csrrw_valid & ~scrw_valid & ~devrd_valid & ~mflag_valid &  gpwa5_valid ) gpwa5_cnt <= gpwa5_cnt + 1; 

           if (rvfi_valid) rvfi_cnt <= rvfi_cnt + 1; 
           if (m_rvfis[0].valid) mrvfi_cnt <= mrvfi_cnt + 1; 

           eot_found_d1 <= (eot_found | eot_found_d1);

           if (eot_found & ~eot_found_d1) begin
               $display("EOT stats: hart=%0d : rvfi_cnt   : %0d", cpu_id, rvfi_cnt);
               $display("EOT stats: hart=%0d : m_rvfi_cnt : %0d", cpu_id, mrvfi_cnt);
               $display("EOT stats: hart=%0d : devrd_cnt  : %0d", cpu_id, devrd_cnt);
               $display("EOT stats: hart=%0d : csrrw_cnt  : %0d", cpu_id, csrrw_cnt);
               $display("EOT stats: hart=%0d : mintr_cnt  : %0d", cpu_id, mintr_cnt);
               $display("EOT stats: hart=%0d : mcm_cnt    : %0d", cpu_id, mcm_cnt);
           end
        end
    end

    assign poke_interrupt = mintr | mtrap_valid | m_interrupt_pends[0].valid | imsic_valid;

    //assign poke_last_uop_event = ((rvfi_last_uop_events != '0) | (rvfi_val_luops != '0)) ? 1'b1 : 1'b0;
    assign poke_last_uop_event = ((rvfi_last_uop_events != '0)) ? 1'b1 : 1'b0;
    assign poke_last_insn_event= ((rvfi_last_insn_events != '0)) ? 1'b1 : 1'b0;
    assign poke_debug_event=  ((psc_off_high !=0) & (clocks >= psc_off_low) & (clocks <= psc_off_high)) ? 1'b1 : 1'b0;

    assign poke_event_out = (poke_events != '0) | send_regs | poke_last_uop_event | poke_last_insn_event | rvfi_debug_mode | poke_debug_event; 

    assign send_rvfi =  poke_event_out | ~PSC_enabled;

    assign m_gp_regss[0].valid      = send_regs;
    assign m_gp_regss[0].data.hart  = NUM;
    assign m_gp_regss[0].data.cycle = clocks; 
    assign m_gp_regss[0].data.location = location;
    for(genvar n=0;n<NGP_REGS;n=n+1) begin
        assign m_gp_regss[0].data.value[n][63:0] = gp_wdata_in[n];
    end

    assign m_fp_regss[0].valid = send_regs & fp_changed;
    assign m_fp_regss[0].data.hart  = NUM;
    assign m_fp_regss[0].data.cycle = clocks; 
    assign m_fp_regss[0].data.location = location;
    for(genvar n=0;n<NFP_REGS;n=n+1) begin
        assign m_fp_regss[0].data.value[n][63:0] = fp_wdata_in[n];
    end

    assign m_vc_regss[0].valid = send_regs & vc_changed;
    assign m_vc_regss[0].data.hart  = NUM;
    assign m_vc_regss[0].data.cycle = clocks; 
    assign m_vc_regss[0].data.location = location;
    for(genvar n=0;n<NVC_REGS;n=n+1) begin
        assign m_vc_regss[0].data.value[n] = vc_wdata_in[n];
    end


    always @(posedge tb_clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            rvfi_enabled = (cvm_plusargs::get_bool("rvfi") != '0) & (location != cvm_topology::nil);
            mcm_enabled = (cvm_plusargs::get_bool("mcm") != '0);
            offline_dpi = (cvm_plusargs::get_bool("offline_dpi") != '0);
            offline_dpi_test = (cvm_plusargs::get_bool("offline_dpi_test") != '0);
            to_host = ((is_eot_tohost() == 1) | (eot_addr != '0));
            poke_mip_timer = (cvm_plusargs::get_bool("poke_mip_timer") != '0);
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

     // m_reset
    logic dut_reset_d1;
    always @(posedge clk) begin
        dut_reset_d1 <= dut_reset;
    end
    assign m_resets[0].valid            = RVFI_EN & rvfi_enabled & (dut_reset_d1 & ~dut_reset);
    assign m_resets[0].data.location    = location;
    assign m_resets[0].data.cycle       = clocks;

    assign m_disable_checkss[0].valid            = RVFI_EN & rvfi_enabled & disable_checks;
    assign m_disable_checkss[0].data.location    = location;
    assign m_disable_checkss[0].data.cycle       = clocks;

    //-----------------------------------------------------------------------------------------------------------
    // PERIODIC STATE COMPARE feature enabled when cosim_period value > 0
    //-----------------------------------------------------------------------------------------------------------
    assign PSC_enabled = (cosim_period != '0) ? 1'b1 : 1'b0;

    // m_rvfi
    for (genvar n = 0; n < NRET; n++) begin
        assign m_rvfis[n].valid            = (RVFI_EN & rvfi_enabled & ~dut_reset & (rvfi[n].valid | rvfi[n].trap) & send_rvfi);
        assign m_rvfis[n].data.location    = location;
        assign m_rvfis[n].data.cycle       = clocks;
        assign m_rvfis[n].data.hart        = NUM;
        assign m_rvfis[n].data.first_uop   = rvfi_first_uop[n];
        assign m_rvfis[n].data.last_uop    = rvfi[n].last_uop;
        assign m_rvfis[n].data.ucode       = rvfi_instr_ucode[n];
        assign m_rvfis[n].data.priv        = rvfi_instr_priv[n];
        assign m_rvfis[n].data.priv_change = rvfi_priv_change[n];
        assign m_rvfis[n].data.set_pmode   = rvfi_set_patch[n];
        assign m_rvfis[n].data.clr_pmode   = rvfi_clr_patch[n];
        assign m_rvfis[n].data.last_insn   = rvfi[n].last_insn;
        assign m_rvfis[n].data.comp        = rvfi[n].comp;
        assign m_rvfis[n].data.order       = rvfi[n].order;
        assign m_rvfis[n].data.branch_tag  = rvfi[n].branch_tag;
        assign m_rvfis[n].data.insn        = rvfi[n].insn;
        assign m_rvfis[n].data.uop         = rvfi[n].uop;
        assign m_rvfis[n].data.trap        = rvfi[n].trap;
        assign m_rvfis[n].data.cause       = rvfi[n].cause;
        assign m_rvfis[n].data.intr        = rvfi[n].intr;
        assign m_rvfis[n].data.mode        = rvfi[n].mode;
        assign m_rvfis[n].data.vec         = rvfi[n].vec;
        assign m_rvfis[n].data.flags_valid = rvfi[n].flags_valid;
        assign m_rvfis[n].data.flags       = rvfi[n].flags;
        assign m_rvfis[n].data.ixl         = rvfi[n].ixl;
        assign m_rvfis[n].data.rd_addr     = rvfi[n].rd_addr;
        assign m_rvfis[n].data.rd_wdata    = rvfi[n].rd_wdata;
        assign m_rvfis[n].data.frd_valid   = rvfi[n].frd_valid;
        assign m_rvfis[n].data.frd_addr    = rvfi[n].frd_addr;
        assign m_rvfis[n].data.frd_wdata   = rvfi[n].frd_wdata;
        assign m_rvfis[n].data.vrd_valid   = rvfi[n].vrd_valid;
        assign m_rvfis[n].data.vrd_addr    = rvfi[n].vrd_addr;
        assign m_rvfis[n].data.vrd_wdata   = rvfi[n].vrd_wdata;
        assign m_rvfis[n].data.csr_valid   = |rvfi[n].csr_wmask;
        assign m_rvfis[n].data.csr_addr    = rvfi[n].csr_addr;
        assign m_rvfis[n].data.csr_wdata   = rvfi[n].csr_wdata;
        assign m_rvfis[n].data.csr_wmask   = rvfi[n].csr_wmask;
        assign m_rvfis[n].data.pc_paddr       = rvfi[n].pc_paddr;
        assign m_rvfis[n].data.pc_rdata    = rvfi[n].pc_rdata;
        assign m_rvfis[n].data.pc_wdata    = rvfi[n].pc_wdata;
        assign m_rvfis[n].data.pc_error    = rvfi[n].pc_error;
        assign m_rvfis[n].data.mem_addr    = rvfi[n].mem_addr;
        assign m_rvfis[n].data.mem_paddr   = rvfi[n].mem_paddr;
        assign m_rvfis[n].data.mem_error   = rvfi[n].mem_error;
        assign m_rvfis[n].data.mem_rmask   = rvfi[n].mem_rmask;
        assign m_rvfis[n].data.mem_rdata   = rvfi[n].mem_rdata;
        assign m_rvfis[n].data.mem_wmask   = rvfi[n].mem_wmask;
        assign m_rvfis[n].data.mem_wdata   = rvfi[n].mem_wdata;
        assign m_rvfis[n].data.mem_attr    = rvfi[n].mem_attr;

        /* verilator lint_off WIDTHTRUNC */
        assign valid_icnt[n]   = valid_count(rvfi_valids, rvfi_orders,rvfi_luops,n+1);    // count of valid-unique rvfi orders from 0 to N+1
        /* verilator lint_on WIDTHTRUNC */
        assign instr_icnt[n]   = instruction_cnt + 64'(valid_icnt[n]);
        assign instr_imax[n]   = ((max_instructions > 0) & (instr_icnt[n] == max_instructions)) ? 1'b1 : 1'b0;

        //--------------------------------------------------------------------------------------------------------------------------------------
        // Logic to generate first_uop, ucode, priv[3:0] and priv_change signals (formerly generated in C++ 
        //--------------------------------------------------------------------------------------------------------------------------------------
        //                               rvfi[0].valid          rvfi[1].valid      rvfi[2].valid      rvfi[7].valid                           //
        //               ____                         |                  |                   |                  |                             //
        //               | V | rvfi_last_uop_S       _|_  last_uop[0]   _|_    last_uop[1]  _|_                _|_                            //
        // last_uop[7]-->|D Q|--------------------->|0  | /----------->|0  | /------------>|0  |         ---->|0  |_last_uop[7]               //
        //               |   | |                    |mux|-/            |mux|-/             |mux|   ****       |mux|  (loops back to FF)       //
        //               |___| |             /----->|1__| |       /--->|1__|    |     /--->|1__|         /--->|1__|                           //
        //                     |             |            |       |             |     |                  |                                    //
        //                     |   rvfi[0].last_uop       |     [1].last_uop  |     [2].last_uop       [7].last_uop                           //
        //                     |            |             |           |    __                                |     __                         //
        //                     |            |             |           \--o|  \__ first_uop[1]                \---o|  \__first_uop[7]          //
        //                     |            |     __      \---------------|__/                       -------------|__/                        //
        //                     |            \---o|  \__first_uop[0]                                                                           //
        //                     \-----------------|__/                                                                                         //
        //                                                                                     Example serial chain logic                     //
        //                                                                                                                                    // 
        // rvfi_ucode:       sequence is a microcode ops                                                                                      // 
        // rvfi_instr_ucode: this value is sent to C++ for instr.ucode value                                                                  // 
        // rvfi_first_uop:   first operation of microcode (this is when rvfi_priv is latched)                                                 // 
        // rvfi_priv:        mode bits loaded into latch on first ucode op (used for comparison needed for patch mode)                        // 
        // rvfi_instr_priv:  non-ucode gets mode bits, ucode sequence it gets latched mode bits on first_uop                                  // 
        // rvfi_priv_change: if priv mode bits change during a microcode sequence then this bit is SET, CLEAREd on last_uop=1                 // 
        // rvfi_set_patch:   if last uop and mode is  4 (patch) then set patch_mode=1                                                         // 
        // rvfi_clr_patch:   if last uop and mode is !4 (but previously was) 4 then set patch_mode=0                                          // 
        //------------------------------------------------------------------------------------------------------------------------------------//

        assign rvfi_mode[n]                = rvfi[n].mode;

        if (n==0) begin
           assign rvfi_ucode[n]               = (rvfi[n].valid) ? ~rvfi[n].last_uop : rvfi_ucode_S;
           assign rvfi_instr_ucode[n]         = ~rvfi[n].last_uop | rvfi_ucode_S ;
           assign rvfi_last_uop[n]            = (rvfi[n].valid) ? rvfi[n].last_uop : rvfi_last_uop_S;
           assign rvfi_first_uop[n]           = (rvfi[n].valid) ? ((~rvfi[n].last_uop) & rvfi_last_uop_S) : 1'b0;
           assign rvfi_priv_change[n]         = (rvfi[n].valid==1'b0) ?  rvfi_priv_change_S :(
                                                   (rvfi_last_uop[n]==1'b1) ? 1'b0 : ( 
                                                   (~rvfi_first_uop[n] & rvfi_instr_ucode[n] & (rvfi_mode[n] != rvfi_priv_S)) ? 1'b1 : rvfi_priv_change_S));
           assign rvfi_priv[n]                = (rvfi[n].valid==1'b0) ? rvfi_priv_S : ((rvfi_last_uop[n] | (rvfi_first_uop[n] & rvfi_instr_ucode[n])) ? rvfi_mode[n] : rvfi_priv_S);  
           assign rvfi_instr_priv[n]          = (~rvfi_instr_ucode[n] | rvfi_first_uop[n]) ? rvfi_mode[n] : rvfi_priv_S;
           assign rvfi_set_patch[n]           = (rvfi[n].valid & rvfi_last_uop[n] & (rvfi_mode[n] == 4'h4) & ~rvfi_patch_mode_S) ? 1'b1 : 1'b0; 
           assign rvfi_clr_patch[n]           = (rvfi[n].valid & rvfi_last_uop[n] & (rvfi_mode[n] != 4'h4) &  rvfi_patch_mode_S) ? 1'b1 : 1'b0;
           assign rvfi_patch_mode[n]          = (rvfi[n].valid==1'b0) ? rvfi_patch_mode_S : (rvfi_clr_patch[n] ? 1'b0 : (rvfi_set_patch[n] ? 1'b1 : rvfi_patch_mode_S)); 
        end
        else begin
           assign rvfi_ucode[n]               = (rvfi[n].valid) ? ~rvfi[n].last_uop : rvfi_ucode[n-1];
           assign rvfi_instr_ucode[n]         = ~rvfi[n].last_uop | rvfi_ucode[n-1] ;
           assign rvfi_last_uop[n]            = (rvfi[n].valid) ? rvfi[n].last_uop : rvfi_last_uop[n-1];
           assign rvfi_first_uop[n]           = (rvfi[n].valid) ? ((~rvfi[n].last_uop) & rvfi_last_uop[n-1]) : 1'b0 ;
           assign rvfi_priv_change[n]         = (rvfi[n].valid==1'b0) ?  rvfi_priv_change[n-1] :(
                                                   (rvfi_last_uop[n]==1'b1) ? 1'b0 : ( 
                                                   (~rvfi_first_uop[n] & rvfi_instr_ucode[n] & (rvfi_mode[n] != rvfi_priv[n-1])) ? 1'b1 : rvfi_priv_change[n-1]));
           assign rvfi_priv[n]                = (rvfi[n].valid==1'b0) ? rvfi_priv[n-1] : ((rvfi_last_uop[n] | (rvfi_first_uop[n] & rvfi_instr_ucode[n])) ? rvfi_mode[n] : rvfi_priv[n-1]);  
           assign rvfi_instr_priv[n]          = (~rvfi_instr_ucode[n] | rvfi_first_uop[n]) ? rvfi_mode[n] : rvfi_priv[n-1];
           assign rvfi_set_patch[n]           = (rvfi[n].valid & rvfi_last_uop[n] & (rvfi_mode[n] == 4'h4) & ~rvfi_patch_mode_S & ~rvfi_set_patch[n-1]) ? 1'b1 : 1'b0; 
           assign rvfi_clr_patch[n]           = (rvfi[n].valid & rvfi_last_uop[n] & (rvfi_mode[n] != 4'h4) & rvfi_patch_mode_S & ~rvfi_clr_patch[n-1]) ? 1'b1 : 1'b0;
           assign rvfi_patch_mode[n]          = (rvfi[n].valid==1'b0) ? rvfi_patch_mode[n-1] : (rvfi_clr_patch[n] ? 1'b0 : (rvfi_set_patch[n] ? 1'b1 : rvfi_patch_mode[n-1])); 
        end
    end   // for loop n

    always @(posedge clk)  begin
        if (dut_reset) begin
           rvfi_ucode_S           <= 1'b0;
           rvfi_last_uop_S        <= 1'b1;
           rvfi_priv_change_S     <= 1'b0;
           rvfi_priv_S            <= 4'b0;
           rvfi_patch_mode_S      <= 1'b0;  // patch-mode enable set by rvfi_set_patch and cleared by rvfi_clr_patch
           rvfi_trap_pmode        <= 1'b0;  // patch-mode flag set by m_trap  and cleared by rvfi_clr_patch
        end
        else begin
           rvfi_ucode_S           <= rvfi_ucode[NRET-1];
           rvfi_last_uop_S        <= rvfi_last_uop[NRET-1];
           rvfi_priv_S            <= rvfi_priv[NRET-1];
           rvfi_priv_change_S     <= rvfi_priv_change[NRET-1];

           if (rvfi_trap_patch != '0) begin
               rvfi_trap_pmode <= 1'b1;
           end
           else begin
               if (rvfi_clr_patch != '0) begin
                  rvfi_trap_pmode     <= 1'b0;
                  rvfi_patch_mode_S   <= 1'b0;
               end
               if (rvfi_set_patch != '0)
                   rvfi_patch_mode_S <= 1'b1;
           end
        end
    end


    assign poke_patch_mode = (rvfi_trap_patch != '0) | rvfi_trap_pmode | (rvfi_set_patch != '0) | (rvfi_clr_patch != '0) | (rvfi_patch_mode != '0); 


    //---------------------------------------------------------------
    // Keep track of number of instructions we did NOT send to cosim
    //  steps will be used to advance the Whisper model
    //
    //  we send a STEP msg to whisper under 2 conditions:
    //       - a POKE event occurred (send_rvfi)
    //       - an instruction order was OUT-of-order...
    //---------------------------------------------------------------

    /* verilator lint_off WIDTHTRUNC */
    assign valid_cnt   = valid_count(rvfi_valids, rvfi_orders,rvfi_luops,NRET);    // count of valid-unique rvfi orders
    /* verilator lint_on WIDTHTRUNC */
    assign rvmax_order = max_order(rvfi_valids, rvfi_orders);                 // highest order valid-unique rvfi

    //----------------------------------------------------------------------
    // order skip is when either order[0] does not equal expected-order  OR
    // when the orders in current RVFI packets is not sequential
    //----------------------------------------------------------------------
    //assign order_skip  = (mcm_enabled & ((rvfi_valids != '0) & (rvfi_skips != '0) & ~rvfi_first_valid)) ? 1'b1 : 1'b0; 
    assign order_skip  = 1'b0; 

    assign val0_order   = rvfi_orders[0]; 
    assign rvfi_skips   = (rvfi_exp_order != rvfi_orders[0]) & ((val0_order > rvfi_exp_order)) ? val0_order - rvfi_exp_order - 1 : '0;               // number of missing orders


    //---------------------------------------------------------------------------------------------------
    // Send steps if:
    //     ARE sending rvfi AND  accumulated-steps > 0
    // OR
    //     NOT sending rvfi AND  we detected an out-of-order RVFI order
    //
    // m_step sequence on the bridge:
    //   - execute steps (increment tag and cycle as needed)
    //   - add skips to tag value
    //   - execute final_steps (only needed if no RVFI packet is being sent)
    //---------------------------------------------------------------------------------------------------
    //assign send_steps   = (send_rvfi & (rvfi_steps != '0)) & rvfi_valid | order_skip | ((poke_event_in & ~poke_event_out) & (rvfi_steps != '0));
    assign send_steps   = (send_rvfi & (rvfi_steps != '0)) ;

    assign m_stepss[0].valid            = RVFI_EN & rvfi_enabled & ~dut_reset & send_steps & PSC_enabled;
    assign m_stepss[0].data.location    = location;
    assign m_stepss[0].data.n_retire    = NRET;
    assign m_stepss[0].data.hart        = NUM;
    assign m_stepss[0].data.cycle       = clocks;
    assign m_stepss[0].data.steps       = rvfi_steps;
    assign m_stepss[0].data.skips       = rvfi_skips;
    assign m_stepss[0].data.final_steps = (send_rvfi==1'b1) ? '0 : 64'(valid_cnt);        // No final steps IF rvfi is being sent too

    always @(posedge clk) begin
        if (dut_reset) begin
            rvfi_steps      <= '0;
            rvfi_scheck_cnt <= '0;
            rvfi_exp_order  <= '0;
            rvfi_first_valid  <= 1'b1;
            instruction_cnt <= '0;
            cycles_since_retire <= '0;
        end
        else begin
            cycles_since_retire <= cycles_since_retire + 1;
            //if (rvfi_valids[0] & rvfi_luops[0]) begin                                    // instruction(s) retiring
            if (rvfi_valid) begin                                    // instruction(s) retiring
                cycles_since_retire <= 0;
                instruction_cnt <= instruction_cnt + 64'(valid_cnt);
                rvfi_first_valid  <= 1'b0;
                rvfi_exp_order <= rvmax_order + 1;                   // compute next clocks rvfi order start to find dropped instruction orders
            end 

            if (send_regs)  
               rvfi_scheck_cnt <= '0;                           // clear counter
            else
            if (rvfi_valid)  
               rvfi_scheck_cnt <= rvfi_scheck_cnt + 32'(valid_cnt);          

            if (rvfi_valid & ~send_steps & ~send_rvfi)                        // we dno not need to STEP whisper  
               rvfi_steps <= rvfi_steps + 64'(valid_cnt);       // increment how many valids we did NOT send
            else
            if (send_steps) 
               rvfi_steps <= 0;                                 // we sent the step-count to whisper .... reset the step counter

        end      
    end

    //-----------------------------------------------------------------------------------------------------------------
    // Periodic COMPARE REGISTER STATE logic:
    //
    //  COSIM RTL:
    //  - A send_regs is issued under 3 conditions:
    //        - We have executed N or more instructions
    //              - and we are not in the middle of a multi-clock op (last_uop==0)
    //        - An event occurs that we need to FORCE an RVFI message
    //              - End of test (EOT)
    //              - MCM poke events (if enabled) 
    //        - A RVFI is writing a register > 31 (reg_waddr5_event)
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

    //assign send_regs = ((rvfi_valid & (rvfi_scheck_cnt >= cosim_period) & (rvfi_val_luops == 0)) | eot_found | reg_waddr5_event) & PSC_enabled & RVFI_EN & rvfi_enabled & ~dut_reset;

    assign send_regs = ((rvfi_valid & (rvfi_scheck_cnt >= cosim_period) & (rvfi_val_luops == 0) & ~rvfi_debug_mode & ~rvfi_trap_pmode) | eot_found) & PSC_enabled & RVFI_EN & rvfi_enabled & ~dut_reset; 
 
    //assign send_regs_i = ((~rvfi_valid & (rvfi_scheck_cnt >= cosim_period))  | eot_found) & PSC_enabled & RVFI_EN & rvfi_enabled & ~dut_reset;

    //assign send_regs = send_regs_i & ~send_regs_d1;

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
    bit [CSR_COUNT-1:0]           csr_ignore_mask;

    assign csr_sel          =   retsel(m_csris_valid);
    assign csr_ignore_mask  =   get_csr_mask();

    //-------------------------------------------------------------------------------------------
    // if csr_sel[MAXCSR] == 1 then we went 1 past the MAX number of DPI calls we can make
    //    only csr_sel[MAXCSR-1:0] are valid.
    //-------------------------------------------------------------------------------------------
    always @(posedge clk) begin
        assert (csr_sel[MAXCSR] == '1) else $error("More than %d CSR valids == 1",MAXCSR-1);
    end


    for (genvar n = 0; n < CSR_COUNT; n++) begin
        assign m_csris_valid[n] = rvfi_enabled & ~dut_reset & csr_ignore_mask[n] &
                                  ((csri[n].valid & ~valid_d1[n]) | 
                                    (csri[n].valid & (((csri[n].data & csri[n].mask) !== (data_d1[n] & mask_d1[n])) |
                                    (csri[n].mask !== mask_d1[n]))));
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
        assign m_mcmi_reads[n].valid = MCMI_EN & mcm_enabled & rvfi_enabled & ~dut_reset & mcmi_read[n].valid;
        assign m_mcmi_reads[n].data.location = location;
        /* verilator lint_off WIDTH */
        assign m_mcmi_reads[n].data.cycle = mcmi_read[n].valid ? clocks : '0;
        /* verilator lint_on WIDTH */
        assign m_mcmi_reads[n].data.hart = NUM;
        assign m_mcmi_reads[n].data.order = mcmi_read[n].order;
        assign m_mcmi_reads[n].data.opcode = mcmi_read[n].opcode;
        assign m_mcmi_reads[n].data.addr = mcmi_read[n].addr;
        assign m_mcmi_reads[n].data.mask = mcmi_read[n].mask;
        assign m_mcmi_reads[n].data.data = mcmi_read[n].data[63:0];
        assign m_mcmi_reads[n].data.data_vec = mcmi_read[n].data[255:0];
        assign m_mcmi_reads[n].data.amo = mcmi_read[n].amo;
        assign m_mcmi_reads[n].data.amo_op = mcmi_read[n].amo_op;
        assign m_mcmi_reads[n].data.v_ext = mcmi_read[n].v_ext;
        assign m_mcmi_reads[n].data.elem_idx = mcmi_read[n].elem_idx;
        assign m_mcmi_reads[n].data.field = mcmi_read[n].field;
        assign m_mcmi_reads[n].data.splat = mcmi_read[n].splat;
        assign m_mcmi_reads[n].data.elem_size = mcmi_read[n].elem_size;
        assign mcmi_read_pokes[n] = mcmi_read[n].valid;

    end



    // m_mcmi_insert
    for (genvar n = 0; n < NINSERT; n++) begin
        assign m_mcmi_inserts[n].valid = MCMI_EN & mcm_enabled & rvfi_enabled & ~dut_reset & mcmi_insert[n].valid;
        assign m_mcmi_inserts[n].data.location = location;
        assign m_mcmi_inserts[n].data.cycle = mcmi_insert[n].valid ? clocks : '0;
        assign m_mcmi_inserts[n].data.hart = NUM;
        assign m_mcmi_inserts[n].data.order = mcmi_insert[n].order;
        assign m_mcmi_inserts[n].data.addr = mcmi_insert[n].addr;
        assign m_mcmi_inserts[n].data.mask = mcmi_insert[n].mask;
        assign m_mcmi_inserts[n].data.data = mcmi_insert[n].data[63:0];
        assign m_mcmi_inserts[n].data.data_vec = mcmi_insert[n].data[255:0];
        assign m_mcmi_inserts[n].data.v_ext = mcmi_insert[n].v_ext;
        assign m_mcmi_inserts[n].data.elem_idx = mcmi_insert[n].elem_idx;
        assign eot_insert_found[n] = ((to_host == 1) & (eot_addr != '0) &  
                                      mcmi_insert[n].valid & (mcmi_insert[n].addr == $bits(mcmi_insert[n].addr)'(eot_addr)) & 
                                      mcmi_insert[n].data[0] & (mcmi_insert[n].data[63:56] == '0)) ? 1'b1 : 1'b0;
        assign eot_insert_data[n] = (eot_write_found[n] == 1'b1) ?  mcmi_insert[n].data[63:0] : '0; 
/* verilator lint_off WIDTHEXPAND */
        assign mcmi_insert_addr[n] = mcmi_insert[n].addr;
/* verilator lint_on WIDTHEXPAND */

        assign mcmi_insert_pokes[n] = mcmi_insert[n].valid;
    end


    // m_mcmi_write
    for (genvar n = 0; n < NWRITE; n++) begin
        assign m_mcmi_writes[n].valid = MCMI_EN & mcm_enabled & rvfi_enabled & ~dut_reset & mcmi_write[n].valid;
        assign m_mcmi_writes[n].data.location = location;
        assign m_mcmi_writes[n].data.cycle = mcmi_write[n].valid ? clocks : '0;
        assign m_mcmi_writes[n].data.hart = NUM;
        assign m_mcmi_writes[n].data.addr = mcmi_write[n].addr;
        assign m_mcmi_writes[n].data.mask = mcmi_write[n].mask;
        assign m_mcmi_writes[n].data.data = mcmi_write[n].data;
        assign m_mcmi_writes[n].data.error = mcmi_write[n].error;

        assign mcmi_write_pokes[n] = mcmi_write[n].valid;

        assign mcmi_write_data[n] = mcmi_write[n].data[63:0]; 
/* verilator lint_off WIDTHEXPAND */
        assign mcmi_write_addr[n] = mcmi_write[n].addr; 
/* verilator lint_on WIDTHEXPAND */
        assign eot_write_found[n] = ((to_host == 1) & (eot_addr != '0) &  
                                      mcmi_write[n].valid & (mcmi_write[n].addr == $bits(mcmi_write[n].addr)'(eot_addr)) & 
                                      mcmi_write[n].data[0] & (mcmi_write[n].data[63:56] == '0)) ? 1'b1 : 1'b0;
        assign eot_write_data[n] = (eot_write_found[n] == 1'b1) ?  mcmi_write[n].data[63:0] : '0; 
    end


    // m_mcmi_bypass
    for (genvar n = 0; n < NBYPASS; n++) begin
        assign m_mcmi_bypasss[n].valid = MCMI_EN & mcm_enabled & rvfi_enabled & ~dut_reset & mcmi_bypass[n].valid;
        assign m_mcmi_bypasss[n].data.location = location;
        assign m_mcmi_bypasss[n].data.cycle = mcmi_bypass[n].valid ? clocks : '0;
        assign m_mcmi_bypasss[n].data.hart = NUM;
        assign m_mcmi_bypasss[n].data.order = mcmi_bypass[n].order;
        assign m_mcmi_bypasss[n].data.addr = mcmi_bypass[n].addr;
        assign m_mcmi_bypasss[n].data.mask = mcmi_bypass[n].mask;
        assign m_mcmi_bypasss[n].data.data = mcmi_bypass[n].data[63:0];
        assign m_mcmi_bypasss[n].data.data_vec = mcmi_bypass[n].data[255:0];
        assign m_mcmi_bypasss[n].data.v_ext = mcmi_bypass[n].v_ext;
        assign m_mcmi_bypasss[n].data.elem_idx = mcmi_bypass[n].elem_idx;
        assign m_mcmi_bypasss[n].data.amo = mcmi_bypass[n].amo;
        assign m_mcmi_bypasss[n].data.amo_op = mcmi_bypass[n].amo_op;
        //-------------------------------------------------------------------------------------------
        // End-Of-Test logic:  memory write to designated address
        //    - will cause a save-state event (force-steps=1 if NO instrs being retired currently
        //-------------------------------------------------------------------------------------------
        assign eot_bypass_found[n] = ((to_host == 1) & (eot_addr != '0) &  
                                      mcmi_bypass[n].valid & (mcmi_bypass[n].addr == $bits(mcmi_bypass[n].addr)'(eot_addr)) & 
                                      mcmi_bypass[n].data[0] & (mcmi_bypass[n].data[63:56] == '0)) ? 1'b1 : 1'b0;
        assign eot_bypass_data[n] = (eot_bypass_found[n]) ? mcmi_bypass[n].data[63:0] : 64'h0; 
/* verilator lint_off WIDTHEXPAND */
        assign mcmi_bypass_addr[n] = mcmi_bypass[n].addr;
/* verilator lint_on WIDTHEXPAND */
        assign mcmi_bypass_pokes[n] = mcmi_bypass[n].valid;

    end

    //----------------------------------------------------------------------------------------------------
    // EOT INTERFACES: only sends packet when it matches an EOT.  Needed for OFFLINE COSIM
    //   eoti-normal  : for normal runs AND offline_dpi replay
    //   eoti-offline : for offline_dpi capture  (only sent during offline_dpi caputure)
    //----------------------------------------------------------------------------------------------------

    assign eot_found      = ~dut_reset & ((eot_write_found != 0) | (eot_bypass_found != 0) | (eot_insert_found != '0) | eot_max_instr) ? 1'b1 : 1'b0; 

    always_comb begin
        eoti_data = '0; 
        for (int n = 0; n < NWRITE; n++) begin
           eoti_data |= eot_write_data[n];
        end
        for (int n = 0; n < NINSERT; n++) begin
           eoti_data |= eot_insert_data[n];
        end
        for (int n = 0; n < NBYPASS; n++) begin
           eoti_data |= eot_bypass_data[n];
        end
    end


    assign m_eoti_normals[0].valid = MCMI_EN &  ~dut_reset & ~offline_dpi_test & eot_found;
    assign m_eoti_normals[0].data.location = location;
    assign m_eoti_normals[0].data.cycle = clocks;
    assign m_eoti_normals[0].data.hart = NUM;
    assign m_eoti_normals[0].data.icount = instr_count;
    assign m_eoti_normals[0].data.data = eoti_data;
    assign m_eoti_normals[0].data.max_instr = eot_max_instr;

    assign m_eoti_offlines[0].valid = MCMI_EN &  ~dut_reset & eot_found & (offline_dpi | offline_dpi_test);
    assign m_eoti_offlines[0].data.location = location;
    assign m_eoti_offlines[0].data.cycle = clocks;
    assign m_eoti_offlines[0].data.hart = NUM;
    assign m_eoti_offlines[0].data.icount = instr_count;
    assign m_eoti_offlines[0].data.data = eoti_data;
    assign m_eoti_offlines[0].data.max_instr = eot_max_instr;


    // m_mcmi_ifetch
    for (genvar n = 0; n < NIFETCH; n++) begin
        assign m_mcmi_ifetch_reqs[n].valid = MCMI_EN & mcm_enabled & rvfi_enabled & ~dut_reset & mcmi_ifetch_req[n].valid;
        assign m_mcmi_ifetch_reqs[n].data.location = location;
        assign m_mcmi_ifetch_reqs[n].data.cycle = mcmi_ifetch_req[n].valid ? clocks : '0;
        assign m_mcmi_ifetch_reqs[n].data.hart = NUM;
        assign m_mcmi_ifetch_reqs[n].data.order = mcmi_ifetch_req[n].order;
        assign m_mcmi_ifetch_reqs[n].data.addr = mcmi_ifetch_req[n].addr;
        assign m_mcmi_ifetch_reqs[n].data.attr = mcmi_ifetch_req[n].attr;
    end

    for (genvar n = 0; n < NIFETCH; n++) begin
        assign m_mcmi_ifetch_resps[n].valid = MCMI_EN & mcm_enabled & rvfi_enabled & ~dut_reset & mcmi_ifetch_resp[n].valid;
        assign m_mcmi_ifetch_resps[n].data.location = location;
        assign m_mcmi_ifetch_resps[n].data.cycle = mcmi_ifetch_resp[n].valid ? clocks : '0;
        assign m_mcmi_ifetch_resps[n].data.hart = NUM;
        assign m_mcmi_ifetch_resps[n].data.order = mcmi_ifetch_resp[n].order;
    end

    // m_mcmi_ievict
    for (genvar n = 0; n < NIEVICT; n++) begin
        assign m_mcmi_ievicts[n].valid = MCMI_EN & mcm_enabled & rvfi_enabled & ~dut_reset & mcmi_ievict[n].valid;
        assign m_mcmi_ievicts[n].data.location = location;
        assign m_mcmi_ievicts[n].data.cycle = mcmi_ievict[n].valid ? clocks : '0;
        assign m_mcmi_ievicts[n].data.hart = NUM;
        assign m_mcmi_ievicts[n].data.addr = mcmi_ievict[n].addr;
        assign mcmi_ievict_pokes[n] = mcmi_ievict[n].valid;
    end

    // m_trap
    logic [63:0] cause_d1, cause_d2, cause_d3;
    always @(posedge clk) begin
      cause_d1 <= rvfi[0].cause;
      cause_d2 <= cause_d1;
      cause_d3 <= cause_d2;
    end
    assign m_traps[0].valid = RVFI_EN & rvfi_enabled & ~dut_reset & (cause_d3 != 0);
    assign m_traps[0].data.location = location;
    assign m_traps[0].data.cycle = clocks;
    assign m_traps[0].data.id = get_trap_id(cause_d3);
    assign m_traps[0].data.cause = cause_d3;
    assign m_traps[0].data.order = rvfi[0].order;
    assign rvfi_trap_patch =  RVFI_EN & rvfi_enabled & ~dut_reset & (cause_d3 != 0) & (cause_d3 >= 58) & ~cause_d3[63];
   
    

    function automatic rv_tester_pkg::trap_e get_trap_id(logic [XLEN-1:0] cause);
      if (cause[63:62] == 'h3)
        return rv_tester_pkg::NMI;
      else if (cause[63:62] == 'h2)
        return rv_tester_pkg::INTR;
      else
        return rv_tester_pkg::EXCP;
    endfunction

    // When using periodic whisper updates... check for eot if max instruction method is used
    assign instr_count = instruction_cnt + 64'(valid_cnt);
    assign eot_max_instr = (instr_imax != '0) ? 1'b1: 1'b0;


    // m_debug
    logic debug_mode_d1;
    logic haltreq_d1;
    always @(posedge clk) begin
      debug_mode_d1 <= debug_mode;
      haltreq_d1 <= haltreq;
    end
    assign m_debugs[0].valid = ~dut_reset & ((debug_mode & ~debug_mode_d1) | (~debug_mode & debug_mode_d1) | (haltreq & ~haltreq_d1) | (~haltreq & haltreq_d1)) & rvfi_enabled;
    assign m_debugs[0].data.location = location;
    assign m_debugs[0].data.cycle = clocks;
    assign m_debugs[0].data.enter = debug_mode;
    assign m_debugs[0].data.exit = ~debug_mode;
    assign m_debugs[0].data.haltreq = haltreq;

    // m_nmi_pend
    rv_tester_pkg::nmi_t nmi_pend_d1;
    always @(posedge clk) begin
      if(dut_reset) begin
        nmi_pend_d1 <= nmi_pend;
      end
      if(~nmi_pend_d1.nmi) begin
        nmi_pend_d1.clai <= nmi_pend.clai;
      end
      if(~nmi_pend_d1.clai) begin
        nmi_pend_d1.nmi <= nmi_pend.nmi;
      end
    end
    assign m_core_nmis[0].valid = ~dut_reset & ((nmi_pend.nmi & ~nmi_pend_d1.nmi) || (nmi_pend.clai & ~nmi_pend_d1.clai) || (~nmi_pend.nmi & nmi_pend_d1.nmi) || (~nmi_pend.clai & nmi_pend_d1.clai)) & rvfi_enabled;
    assign m_core_nmis[0].data.location = location;
    assign m_core_nmis[0].data.cycle = clocks;
    assign m_core_nmis[0].data.nmi_assert = (nmi_pend.nmi & ~nmi_pend_d1.nmi & ~nmi_pend.clai) || (nmi_pend.clai & ~nmi_pend_d1.clai & ~nmi_pend.nmi);
    assign m_core_nmis[0].data.nmi_cause = (nmi_pend.nmi & ~nmi_pend_d1.nmi & ~nmi_pend.clai) ? 2 : ((nmi_pend.clai & ~nmi_pend_d1.clai & ~nmi_pend.nmi) ? 3 : 0);

    function automatic bit [63:0] get_nmi_cause(rv_tester_pkg::nmi_t n);
      bit [63:0] cause = '0;
      if (n.nmi)
        cause = 2;
      else if (n.clai)
        cause = 3;
      return cause;
    endfunction

    // m_interrupt_pend
    logic [63:0] mip_d1, mip_timer, mip_timer_d1;
    logic seip_d1;
    assign mip_timer = interrupt_pend.mip & 'he0;
    always @(posedge clk) begin
      mip_timer_d1 <= mip_timer;
      mip_d1 <= interrupt_pend.mip;
      seip_d1 <= interrupt_pend.seip;
    end
    assign m_interrupt_pends[0].valid = ~dut_reset & interrupt_pend.valid & rvfi_enabled;
    assign m_interrupt_pends[0].data.location = location;
    assign m_interrupt_pends[0].data.cycle = clocks;
    assign m_interrupt_pends[0].data.hw = interrupt_pend.hw;
    assign m_interrupt_pends[0].data.mip = interrupt_pend.mip;
    assign m_interrupt_pends[0].data.mip_set = interrupt_pend.mip & ~mip_d1;
    assign m_interrupt_pends[0].data.mip_clr = ~interrupt_pend.mip & mip_d1;
    assign m_interrupt_pends[0].data.seip = interrupt_pend.seip;
    assign m_interrupt_pends[0].data.seip_set = interrupt_pend.seip & ~seip_d1;
    assign m_interrupt_pends[0].data.seip_clr = ~interrupt_pend.seip & seip_d1;

    // m_imsic_msi
    assign m_imsic_msis[0].valid = ~dut_reset && imsic_msi.valid && rvfi_enabled;
    assign m_imsic_msis[0].data.location = location;
    assign m_imsic_msis[0].data.cycle = clocks;
    assign m_imsic_msis[0].data.addr = imsic_msi.addr;
    assign m_imsic_msis[0].data.data = imsic_msi.data;

    // m_mtime
    localparam logic [CSRLEN-1:0] C_TIME       = 'hC01;
    localparam logic [CSRLEN-1:0] C_STIMECMP   = 'h14D;
    localparam logic [CSRLEN-1:0] C_VSTIMECMP  = 'h24D;
    localparam logic [CSRLEN-1:0] C_HTIMEDELTA = 'h605;
    localparam logic [CSRLEN-1:0] C_MIP        = 'h344;
    localparam logic [CSRLEN-1:0] C_SIP        = 'h144;
    localparam logic [CSRLEN-1:0] C_VSIP       = 'h244;
    localparam logic [CSRLEN-1:0] C_MTOPI      = 'hFB0;
    localparam logic [CSRLEN-1:0] C_STOPI      = 'hDB0;
    localparam logic [CSRLEN-1:0] C_VSTOPI     = 'hEB0;

    // mtime packets from csr reads/writes
    for (genvar n = 0; n < NRET; n++) begin
      assign m_mtimes[n].valid = ~dut_reset && rvfi_enabled && !poke_mip_timer && (rvfi[n].valid &&
         ((|rvfi[n].csr_wmask && (rvfi[n].csr_addr inside {C_STIMECMP, C_VSTIMECMP, C_HTIMEDELTA})) ||
          (|rvfi[n].csr_rmask && (rvfi[n].csr_addr inside {C_TIME, C_MIP, C_MTOPI, C_SIP, C_STOPI, C_VSIP, C_VSTOPI}))));
      assign m_mtimes[n].data.location = location;
      assign m_mtimes[n].data.cycle = clocks;
      assign m_mtimes[n].data.mtime = mtime;
      assign m_mtimes[n].data.mip = ((64'(rvfi[n].csr_addr inside {C_STIMECMP})) << 5) | (64'((rvfi[n].csr_addr inside {C_VSTIMECMP, C_HTIMEDELTA})) << 6);
    end

    // mtime packets from mip bits
      assign m_mtimes[NRET].valid = ~dut_reset && rvfi_enabled && |(mip_timer & ~mip_timer_d1);
      assign m_mtimes[NRET].data.location = location;
      assign m_mtimes[NRET].data.cycle = clocks;
      assign m_mtimes[NRET].data.mtime = mtime;
      assign m_mtimes[NRET].data.mip = mip_timer;

    //--------------------------------------------------------------------
    // set debug entry/exit values to defaults it NOT specificed by user
    //--------------------------------------------------------------------
    assign debug_entry_pc = (debug_entry_pc_arg != '0) ? PA_WIDTH'(debug_entry_pc_arg) : debug_entry_pc_const;
    assign debug_exit_pc  = (debug_exit_pc_arg != '0)  ? PA_WIDTH'(debug_exit_pc_arg) : debug_exit_pc_const; 

/* verilator lint_off WIDTHEXPAND */
    assign hart = NUM;
/* verilator lint_on WIDTHEXPAND */

    localparam bit [63:0] DRAM_BASE = 64'h8000_0000;
    always @(posedge tb_clk) begin
      if (reset) begin
        /* verilator lint_off BLKSEQ */
        max_cycle = cvm_plusargs::get_ulongint("max_cycle");
        max_stall_cycle = cvm_plusargs::get_int("max_stall_cycle");
        cosim_period = cvm_plusargs::get_int("cosim_period");
        max_instructions = cvm_plusargs::get_ulongint("max_instr");
        nharts = cvm_plusargs::get_int("num_harts");
        hart_enable_mask = cvm_plusargs::get_int("hart_enable_mask");
        debug_entry_pc_arg = cvm_plusargs::get_ulongint("debug_entry_pc");
        debug_exit_pc_arg  = cvm_plusargs::get_ulongint("debug_exit_pc");
        //mcm_value  = cvm_plusargs::get_int("mcm");
        psc_off_low  = cvm_plusargs::get_ulongint("psc_off_low");
        psc_off_high = cvm_plusargs::get_ulongint("psc_off_high");

        /* verilator lint_on BLKSEQ */
        boot_wfi <= '0;
        cosim_terminate_sent <= '0;
        boot_done <= '0;
      end else if(!reset) begin
        if (NUM != 0 && rvfi[0].valid == '1 && rvfi[0].insn[6:0] == 7'h73 && rvfi[0].pc_rdata < 'h20000) begin // WFI
          boot_wfi <= '1;
        end
        if (rvfi[0].valid == '1 && rvfi[0].pc_rdata == DRAM_BASE) begin
          boot_done <= '1;
        end
        if (max_stall_cycle > 0 && cycles_since_retire > max_stall_cycle && !boot_wfi && NUM < nharts && cosim_terminate_sent == '0) begin
          $display("\nError: Hart %0d: No instruction retired for max_stall_cycle (%0d) cycles", NUM, max_stall_cycle);
          cosim_terminate();
          cosim_terminate_sent <= '1;
        end
        if (max_cycle > 0 && clocks > max_cycle && NUM < nharts && cosim_terminate_sent == '0) begin
          $display("\nError: Hart %0d:  Test running for max_cycle (%0d) cycles - stuck in a loop, or too long", NUM, max_cycle);
          cosim_terminate();
          cosim_terminate_sent <= '1;
        end
        if (rvfi[0].valid == '1 && NUM > nharts && cosim_terminate_sent == '0) begin
          $display("\nError: Core %0d: Instruction retire seen on disabled/harvested core", NUM);
          cosim_terminate();
          cosim_terminate_sent <= '1;
        end
      end
    end

endmodule
