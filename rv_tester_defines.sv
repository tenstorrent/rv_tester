///////////////includes///////////////////////////////

`include "axi/typedef.svh"

/////////////////////////////////////////////////////

package rv_tester_params;

    import cvm_topology_gen::mods;

    // --------------------------------------
    // Platform RISCV Arch
    // --------------------------------------
    parameter NHARTS = mods.TOP.PLATFORM.NHARTS;
    parameter HARTLEN = 16; //NHARTS == 1 ? 1 : $clog2(NHARTS);
    parameter bit RVF = mods.TOP.PLATFORM.ISA_RVF == 1;
    parameter bit RVD = mods.TOP.PLATFORM.ISA_RVD == 1;
    parameter bit RVFD = RVF | RVD;
    parameter bit RVV = mods.TOP.PLATFORM.ISA_RVV == 1;
    parameter bit RVC = mods.TOP.PLATFORM.ISA_RVC == 1;
    parameter ILEN = mods.TOP.PLATFORM.ILEN;
    parameter XLEN = mods.TOP.PLATFORM.XLEN;
    parameter FLEN = RVD ? 64 : RVF ? 32 : 1;
    parameter VLEN = RVV ? mods.TOP.PLATFORM.VLEN : 1;
    parameter RDLEN = mods.TOP.PLATFORM.RDLEN;
    parameter VALEN = mods.TOP.PLATFORM.VALEN;
    parameter PALEN = mods.TOP.PLATFORM.PALEN;
    parameter CLLEN = mods.TOP.PLATFORM.CLLEN;
    parameter CSRLEN = mods.TOP.PLATFORM.CSRLEN;

    // --------------------------------------
    // Clk interface
    // --------------------------------------
    parameter NCLKS = mods.TOP.PLATFORM.CLKI.NCLKS;
    parameter TB_CLK_IDX = mods.TOP.PLATFORM.CLKI.TB_CLK_IDX;
    parameter REF_CLK_IDX = mods.TOP.PLATFORM.CLKI.REF_CLK_IDX;
    parameter CORE_CLK_IDX = mods.TOP.PLATFORM.CLKI.CORE_CLK_IDX;
    parameter AXI_CLK_IDX = mods.TOP.PLATFORM.CLKI.AXI_CLK_IDX;
    parameter SOC_CLK_IDX = mods.TOP.PLATFORM.CLKI.SOC_CLK_IDX;
    parameter SW_CLOCK_PERIOD_PS = mods.TOP.PLATFORM.CLKI.SW_CLOCK_PERIOD_PS;
    parameter bit [NCLKS-1:0][31:0] CLOCK_FREQ_MHZ = mods.TOP.PLATFORM.CLKI.CLOCK_FREQ_MHZ;
    parameter bit [NCLKS-1:0][31:0] PROFILE1_CLOCK_FREQ_MHZ = mods.TOP.PLATFORM.CLKI.PROFILE1_CLOCK_FREQ_MHZ;
    parameter bit [NCLKS-1:0][31:0] PROFILE2_CLOCK_FREQ_MHZ = mods.TOP.PLATFORM.CLKI.PROFILE2_CLOCK_FREQ_MHZ;
    parameter bit [NCLKS-1:0][31:0] PROFILE3_CLOCK_FREQ_MHZ = mods.TOP.PLATFORM.CLKI.PROFILE3_CLOCK_FREQ_MHZ;
    parameter bit [NCLKS-1:0][31:0] PROFILE4_CLOCK_FREQ_MHZ = mods.TOP.PLATFORM.CLKI.PROFILE4_CLOCK_FREQ_MHZ;
    parameter bit [NCLKS-1:0][31:0] PROFILE5_CLOCK_FREQ_MHZ = mods.TOP.PLATFORM.CLKI.PROFILE5_CLOCK_FREQ_MHZ;
    parameter bit [NCLKS-1:0][31:0] PROFILE6_CLOCK_FREQ_MHZ = mods.TOP.PLATFORM.CLKI.PROFILE6_CLOCK_FREQ_MHZ;

    // --------------------------------------
    // Reset interface
    // --------------------------------------
    parameter NRESETS = mods.TOP.PLATFORM.RESETI.NRESETS;
    parameter COLD_RESET_IDX = mods.TOP.PLATFORM.RESETI.COLD_RESET_IDX;
    parameter WARM_RESET_IDX = mods.TOP.PLATFORM.RESETI.WARM_RESET_IDX;
    parameter NHOLDS = mods.TOP.PLATFORM.RESETI.NHOLDS;
    parameter SRAM_HOLD_IDX = mods.TOP.PLATFORM.RESETI.SRAM_HOLD_IDX;
    parameter DEBUG_HOLD_IDX = mods.TOP.PLATFORM.RESETI.DEBUG_HOLD_IDX;
    parameter CRITICAL_HOLD_IDX = mods.TOP.PLATFORM.RESETI.CRITICAL_HOLD_IDX;

    // --------------------------------------
    // AXI interface
    // --------------------------------------
    parameter AXI_TOTAL = mods.TOP.PLATFORM.AXI.TOTAL;
    parameter AXI_ADDR_WIDTH = mods.TOP.PLATFORM.AXI.ADDR_WIDTH;
    parameter AXI_DATA_WIDTH = mods.TOP.PLATFORM.AXI.DATA_WIDTH;
    parameter AXI_STRB_WIDTH = mods.TOP.PLATFORM.AXI.STRB_WIDTH;
    parameter AXI_ID_WIDTH = mods.TOP.PLATFORM.AXI.ID_WIDTH;

    parameter NCIO_AXI_TOTAL = mods.TOP.PLATFORM.NCIO_AXI.TOTAL;
    parameter NCIO_AXI_ADDR_WIDTH = mods.TOP.PLATFORM.NCIO_AXI.ADDR_WIDTH;
    parameter NCIO_AXI_DATA_WIDTH = mods.TOP.PLATFORM.NCIO_AXI.DATA_WIDTH;
    parameter NCIO_AXI_STRB_WIDTH = mods.TOP.PLATFORM.NCIO_AXI.STRB_WIDTH;
    parameter NCIO_AXI_ID_WIDTH = mods.TOP.PLATFORM.NCIO_AXI.ID_WIDTH;

    parameter AXI_MST_TOTAL = mods.TOP.PLATFORM.AXI_MST.TOTAL;
    parameter AXI_MST_ADDR_WIDTH = mods.TOP.PLATFORM.AXI_MST.ADDR_WIDTH;
    parameter AXI_MST_DATA_WIDTH = mods.TOP.PLATFORM.AXI_MST.DATA_WIDTH;
    parameter AXI_MST_STRB_WIDTH = mods.TOP.PLATFORM.AXI_MST.STRB_WIDTH;
    parameter AXI_MST_ID_WIDTH = mods.TOP.PLATFORM.AXI_MST.ID_WIDTH;
    parameter AXI_MST_USER_WIDTH = mods.TOP.PLATFORM.AXI_MST.USER_WIDTH;

    parameter SMC_AXI_MST_TOTAL = mods.TOP.PLATFORM.SMC_AXI_MST.TOTAL;
    parameter SMC_AXI_MST_ADDR_WIDTH = mods.TOP.PLATFORM.SMC_AXI_MST.ADDR_WIDTH;
    parameter SMC_AXI_MST_DATA_WIDTH = mods.TOP.PLATFORM.SMC_AXI_MST.DATA_WIDTH;
    parameter SMC_AXI_MST_STRB_WIDTH = mods.TOP.PLATFORM.SMC_AXI_MST.STRB_WIDTH;
    parameter SMC_AXI_MST_ID_WIDTH = mods.TOP.PLATFORM.SMC_AXI_MST.ID_WIDTH;
    parameter SMC_AXI_MST_USER_WIDTH = mods.TOP.PLATFORM.SMC_AXI_MST.USER_WIDTH;

    parameter DM_AXI_TOTAL = mods.TOP.PLATFORM.DM_AXI.TOTAL;
    parameter DM_AXI_ADDR_WIDTH = mods.TOP.PLATFORM.DM_AXI.ADDR_WIDTH;
    parameter DM_AXI_DATA_WIDTH = mods.TOP.PLATFORM.DM_AXI.DATA_WIDTH;
    parameter DM_AXI_STRB_WIDTH = mods.TOP.PLATFORM.DM_AXI.STRB_WIDTH;
    parameter DM_AXI_ID_WIDTH = mods.TOP.PLATFORM.DM_AXI.ID_WIDTH;

    typedef logic [AXI_ADDR_WIDTH-1:0] axi_addr_t;
    typedef logic [AXI_DATA_WIDTH-1:0] axi_data_t;
    typedef logic [AXI_STRB_WIDTH-1:0] axi_strb_t;
    typedef logic [AXI_ID_WIDTH  -1:0] axi_id_t;

    typedef logic [NCIO_AXI_ADDR_WIDTH-1:0] ncio_axi_addr_t;
    typedef logic [NCIO_AXI_DATA_WIDTH-1:0] ncio_axi_data_t;
    typedef logic [NCIO_AXI_STRB_WIDTH-1:0] ncio_axi_strb_t;
    typedef logic [NCIO_AXI_ID_WIDTH  -1:0] ncio_axi_id_t;

    typedef logic [AXI_MST_ADDR_WIDTH-1:0] axi_mst_addr_t;
    typedef logic [AXI_MST_DATA_WIDTH-1:0] axi_mst_data_t;
    typedef logic [AXI_MST_STRB_WIDTH-1:0] axi_mst_strb_t;
    typedef logic [AXI_MST_ID_WIDTH  -1:0] axi_mst_id_t;
    typedef logic [AXI_MST_USER_WIDTH-1:0] axi_mst_user_t;

    typedef logic [SMC_AXI_MST_ADDR_WIDTH-1:0] smc_addr_t;
    typedef logic [SMC_AXI_MST_DATA_WIDTH-1:0] smc_data_t;
    typedef logic [SMC_AXI_MST_STRB_WIDTH-1:0] smc_strb_t;
    typedef logic [SMC_AXI_MST_ID_WIDTH  -1:0] smc_id_t;
    typedef logic [SMC_AXI_MST_USER_WIDTH-1:0] smc_user_t;

    typedef logic [5:0] axi_atop_t;
    typedef logic [1:0] axi_burst_t;
    typedef logic [1:0] axi_resp_t;
    typedef logic [7:0] axi_len_t;
    typedef logic [2:0] axi_size_t;

    typedef struct packed {
        logic                   ar_valid ;
        axi_id_t                ar_id    ;
        axi_addr_t              ar_addr  ;
        axi_len_t               ar_len   ;
        axi_size_t              ar_size  ;
        axi_burst_t             ar_burst ;
        logic                   ar_lock  ;

        logic                   aw_valid ;
        axi_id_t                aw_id    ;
        axi_addr_t              aw_addr  ;
        axi_len_t               aw_len   ;
        axi_size_t              aw_size  ;
        axi_burst_t             aw_burst ;
        logic                   aw_lock  ;
        axi_atop_t              aw_atop  ;

        logic                   w_valid  ;
        axi_data_t              w_data   ;
        axi_strb_t              w_strb   ;
        logic                   w_last   ;

        logic                   b_ready  ;
        logic                   r_ready  ;
    } axi_req_t;

    typedef struct packed {
        logic                   b_valid  ;
        axi_id_t                b_id     ;
        axi_resp_t              b_resp   ;

        logic                   r_valid  ;
        axi_id_t                r_id     ;
        axi_data_t              r_data   ;
        axi_resp_t              r_resp   ;
        logic                   r_last   ;

        logic                   aw_ready ;
        logic                   ar_ready ;
        logic                   w_ready  ;
    } axi_rsp_t;

    typedef struct packed {
        logic                       ar_valid ;
        axi_mst_id_t                ar_id    ;
        axi_mst_addr_t              ar_addr  ;
        axi_len_t                   ar_len   ;
        axi_size_t                  ar_size  ;
        axi_burst_t                 ar_burst ;
        logic                       ar_lock  ;

        logic                       aw_valid ;
        axi_mst_id_t                aw_id    ;
        axi_mst_addr_t              aw_addr  ;
        axi_len_t                   aw_len   ;
        axi_size_t                  aw_size  ;
        axi_burst_t                 aw_burst ;
        logic                       aw_lock  ;
        axi_atop_t                  aw_atop  ;

        logic                       w_valid  ;
        axi_mst_data_t              w_data   ;
        axi_mst_strb_t              w_strb   ;
        logic                       w_last   ;

        logic                       b_ready  ;
        logic                       r_ready  ;
    } axi_req_mst_t;

    typedef struct packed {
        logic                       b_valid  ;
        axi_mst_id_t                b_id     ;
        axi_resp_t                  b_resp   ;

        logic                       r_valid  ;
        axi_mst_id_t                r_id     ;
        axi_mst_data_t              r_data   ;
        axi_resp_t                  r_resp   ;
        logic                       r_last   ;

        logic                       aw_ready ;
        logic                       ar_ready ;
        logic                       w_ready  ;
    } axi_rsp_mst_t;

    typedef struct packed {
        logic                       ar_valid ;
        smc_id_t                    ar_id    ;
        smc_addr_t                  ar_addr  ;
        axi_len_t                   ar_len   ;
        axi_size_t                  ar_size  ;
        axi_burst_t                 ar_burst ;
        logic                       ar_lock  ;

        logic                       aw_valid ;
        smc_id_t                    aw_id    ;
        smc_addr_t                  aw_addr  ;
        axi_len_t                   aw_len   ;
        axi_size_t                  aw_size  ;
        axi_burst_t                 aw_burst ;
        logic                       aw_lock  ;
        axi_atop_t                  aw_atop  ;

        logic                       w_valid  ;
        smc_data_t                  w_data   ;
        smc_strb_t                  w_strb   ;
        logic                       w_last   ;

        logic                       b_ready  ;
        logic                       r_ready  ;
    } smc_axi_req_t;

    typedef struct packed {
        logic                       b_valid  ;
        smc_id_t                    b_id     ;
        axi_resp_t                  b_resp   ;

        logic                       r_valid  ;
        smc_id_t                    r_id     ;
        smc_data_t                  r_data   ;
        axi_resp_t                  r_resp   ;
        logic                       r_last   ;

        logic                       aw_ready ;
        logic                       ar_ready ;
        logic                       w_ready  ;
    } smc_axi_rsp_t;


    // --------------------------------------
    // Bootstrap
    // --------------------------------------
    typedef struct packed {
        logic [VALEN-1:0]           boot_addr;
    } bootstrap_t;

    // --------------------------------------
    // Interrupts
    // --------------------------------------
    typedef struct packed {
        logic                      valid;
        logic                      hw;
        logic [XLEN-1:0]           mip;
        logic                      seip;
        logic [5:0]                buserr_bit;
    } interrupt_pend_t;

    // --------------------------------------
    // RVFI
    // --------------------------------------
    parameter bit RVFI_EN = mods.TOP.PLATFORM.COSIM.RVFI.ENABLE == 1;
    parameter bit RVFI_FP = RVFI_EN & RVFD & mods.TOP.PLATFORM.COSIM.RVFI.FP_ENABLE == 1;
    parameter bit RVFI_RVV = RVFI_EN & RVV & mods.TOP.PLATFORM.COSIM.RVFI.VEC_ENABLE == 1;
    parameter bit RVFI_UOP = RVFI_EN & mods.TOP.PLATFORM.COSIM.RVFI.UOP_ENABLE == 1;
    parameter bit RVFI_MEM = RVFI_EN & mods.TOP.PLATFORM.COSIM.RVFI.MEM_ENABLE == 1;
    parameter bit RVFI_CSR = RVFI_EN & mods.TOP.PLATFORM.COSIM.RVFI.CSR_ENABLE == 1;
    parameter int TOTAL_NRETS = RVFI_EN ? mods.TOP.PLATFORM.COSIM.RVFI.TOTAL_NRETS : 1;
    parameter bit [NHARTS-1:0][31:0] NRETS = mods.TOP.PLATFORM.COSIM.RVFI.NRETS;
    parameter bit [NHARTS-1:0][31:0] NRETS_CUMSUM    = mods.TOP.PLATFORM.COSIM.RVFI.NRETS_CUMSUM;

    typedef struct packed {
        logic                       valid    ;
        logic [HARTLEN-1:0]         hart     ;
        logic [64-1:0]              order    ;
        logic [64-1:0]              branch_tag;
        logic [ILEN-1:0]            insn     ;
        logic                       trap     ;
        logic [XLEN-1:0]            cause    ;
        logic                       halt     ;
        logic                       intr     ;
        logic                       virt_mode;
        logic [4-1:0]               mode     ;
        logic [2-1:0]               ixl      ;
        logic [6-1:0]               rd_addr  ;
        logic [XLEN-1:0]            rd_wdata ;
        logic                       frd_valid;
        logic [6-1:0]               frd_addr ;
        logic [FLEN-1:0]            frd_wdata;
        logic                       vrd_valid;
        logic [6-1:0]               vrd_addr ;
        logic [VLEN-1:0]            vrd_wdata;
        logic                       vec      ;
        logic                       flags_valid;
        logic [5-1:0]               flags    ;
        logic [CSRLEN-1:0]          csr_addr ;
        logic [XLEN-1:0]            csr_wdata;
        logic [XLEN-1:0]            csr_wmask;
        logic [XLEN-1:0]            csr_rdata;
        logic [XLEN-1:0]            csr_rmask;
        logic [VALEN-1:0]           pc_paddr ;
        logic [VALEN-1:0]           pc_rdata ;
        logic [VALEN-1:0]           pc_wdata ;
        logic                       pc_error ;
        logic [VALEN-1:0]           mem_addr ;
        logic [PALEN-1:0]           mem_paddr;
        logic [(RDLEN/8)-1:0]       mem_rmask;
        logic [(RDLEN/8)-1:0]       mem_wmask;
        logic [RDLEN-1:0]           mem_rdata;
        logic [RDLEN-1:0]           mem_wdata;
        logic [32-1:0]              mem_attr ;
        logic                       mem_error;
        logic                       comp     ;
        logic [64-1:0]              uop      ;
        logic                       last_uop ;
        logic                       last_insn;
    } rvfi_t;

    // --------------------------------------
    // MCMI - Memory Consistency Model Checks
    // --------------------------------------
    parameter bit MCMI_EN = mods.TOP.PLATFORM.COSIM.MCMI.ENABLE == 1;
    parameter TOTAL_NREADS = MCMI_EN ? mods.TOP.PLATFORM.COSIM.MCMI.TOTAL_NREADS : 1;
    parameter TOTAL_NINSERTS = MCMI_EN ? mods.TOP.PLATFORM.COSIM.MCMI.TOTAL_NINSERTS : 1;
    parameter TOTAL_NWRITES = MCMI_EN ? mods.TOP.PLATFORM.COSIM.MCMI.TOTAL_NWRITES : 1;
    parameter TOTAL_NBYPASSES = MCMI_EN ? mods.TOP.PLATFORM.COSIM.MCMI.TOTAL_NBYPASSES : 1;
    parameter TOTAL_NIFETCHES = MCMI_EN ? mods.TOP.PLATFORM.COSIM.MCMI.TOTAL_NIFETCHES : 1;
    parameter TOTAL_NIEVICTS = MCMI_EN ? mods.TOP.PLATFORM.COSIM.MCMI.TOTAL_NIEVICTS : 1;
    parameter bit [NHARTS-1:0][31:0] NREADS = mods.TOP.PLATFORM.COSIM.MCMI.NREADS;
    parameter bit [NHARTS-1:0][31:0] NINSERTS = mods.TOP.PLATFORM.COSIM.MCMI.NINSERTS;
    parameter bit [NHARTS-1:0][31:0] NWRITES = mods.TOP.PLATFORM.COSIM.MCMI.NWRITES;
    parameter bit [NHARTS-1:0][31:0] NBYPASSES = mods.TOP.PLATFORM.COSIM.MCMI.NBYPASSES;
    parameter bit [NHARTS-1:0][31:0] NIFETCHES = mods.TOP.PLATFORM.COSIM.MCMI.NIFETCHES;
    parameter bit [NHARTS-1:0][31:0] NIEVICTS = mods.TOP.PLATFORM.COSIM.MCMI.NIEVICTS;
    parameter bit [NHARTS-1:0][31:0] NREADS_CUMSUM   = mods.TOP.PLATFORM.COSIM.MCMI.NREADS_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NINSERTS_CUMSUM = mods.TOP.PLATFORM.COSIM.MCMI.NINSERTS_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NWRITES_CUMSUM  = mods.TOP.PLATFORM.COSIM.MCMI.NWRITES_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NBYPASSES_CUMSUM  = mods.TOP.PLATFORM.COSIM.MCMI.NBYPASSES_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NIFETCHES_CUMSUM  = mods.TOP.PLATFORM.COSIM.MCMI.NIFETCHES_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NIEVICTS_CUMSUM  = mods.TOP.PLATFORM.COSIM.MCMI.NIEVICTS_CUMSUM;

    typedef struct packed {
        logic                       valid;
        logic [15:0]                hart ;
        logic [64-1:0]              cycle;
        logic [64-1:0]              order;
        logic [PALEN-1:0]           addr ;
        logic [(CLLEN/8)-1:0]       mask ;
        logic [CLLEN-1:0]           data ;
        logic [31:0]                attr ;
        logic                       error;
        logic                       cbo  ;
        logic                       amo  ;
        logic [4:0]                 amo_op;
        logic                       v_ext;
        logic [36-1:0]              opcode;
        logic [7:0]                 field;
        logic [7:0]                 elem_idx;
        logic                       splat;
        logic [7:0]                 elem_size;
    } mcmi_t;

    // --------------------------------------
    // AIA - IMSIC message signaled interrupt
    // --------------------------------------
    typedef struct packed {
        logic                       valid;
        logic [15:0]                hart ;
        logic [63:0]                cycle;
        logic [63:0]                addr ;
        logic [31:0]                data ;
    } msi_t;

    // --------------------------------------
    // ACLINT - Advanced Core Interrupt
    // --------------------------------------
    typedef struct packed {
        logic [63:0] data;
        logic valid;
    } ac_cr_sync;
    typedef struct packed {
        logic [23:0] addr;
        logic [63:0] data;
        logic [7:0] mask;
        logic [3:0] srcid;
        logic [7:0] user;
        logic valid;
    } cr_ac_axi_pkt;
    // --------------------------------------
    // C2 
    // --------------------------------------
    typedef enum {C2,PATCH,UARCH_INTR,TRIGGER_COUNT} event_trigger_type_t;
    typedef struct packed {
        logic [63:0] data;
        logic [63:0] addr;
        logic valid;
    } event_trigger_t;
    typedef event_trigger_t [TRIGGER_COUNT-1:0] event_trigger_intf_t;
    // --------------------------------------
    // CSRI - Control Status Registers
    // --------------------------------------
    parameter bit [31:0] MAX_NCSRI = mods.TOP.PLATFORM.CSRI.NCSRI;

    typedef struct packed {
    logic [CSRLEN-1:0] addr;
    logic [XLEN-1:0]   mask;
    logic [XLEN-1:0]   data;
    } csri_data_pkt_t;

    typedef struct packed {
        csri_data_pkt_t data; 
        logic           valid;
    } csr_entry_t;

    typedef csr_entry_t [MAX_NCSRI-1:0] csri_t;

    // --------------------------------------
    // PMCI - Performance Monitoring Counters
    // --------------------------------------
    parameter bit PMCI_EN = mods.TOP.PLATFORM.PMCI.ENABLE == 1;
    parameter bit [NHARTS-1:0][31:0] LS_PIPES = mods.TOP.PLATFORM.PMCI.LS_PIPES;

     //AUTOGENERATED -- NOTOUCH 
    typedef enum {
        CPU_CYCLES,
        INSTRUCTIONS,
        M_MODE_CYCLES,
        M_MODE_INSTRET,
        S_MODE_CYCLES,
        S_MODE_INSTRET,
        U_MODE_CYCLES,
        U_MODE_INSTRET,
        REF_CPU_CYCLES,
        STALLS_BST_FULL,
        STALLS_PFX_FULL,
        NFP_EARLY_REDIRECT,
        NFP_LATE_REDIRECT,
        STALLS_INDIRECT_MISS,
        STALLS_ICACHE_MISS,
        STALLS_ITLB_MISS,
        STALLS_EXCEPTION,
        STALLS_IRB_FULL,
        STALLS_IFBUF_FULL,
        PAGE_CROSSING_FETCHBLOCKS,
        IFBUF_FULL_REDIRECT,
        FAULT_RESYNC,
        FAULT_REFETCH,
        CMODE_ENTRY,
        BRANCH_MISSES,
        BR_RET_MISSES,
        IND_BR_MISSES,
        REL_BR_MISSES,
        SPEC_BRANCH_REDIRECT,
        SPEC_LSU_RESYNCS,
        TOTAL_FLUSHES,
        TOTAL_TRAPS,
        BST_FULL_ON_EX_REDIRECT,
        PFX_FULL_ON_EX_REDIRECT,
        L1I_READ_ACCESS,
        L1I_READ_MISS,
        L1I_PREFETCH_ACCESS,
        L1I_PREFETCH_MISS,
        ITLB_READ_ACCESS,
        ITLB_READ_MISS,
        ITLB_PREFETCH_ACCESS,
        ITLB_PREFETCH_MISS,
        IC_WAY_MISPRED,
        RAS_UNDERFLOW,
        RAS_OVERFLOW,
        NUM_FETCHGROUPS,
        BDP_BANK_CONFLICTS,
        BTP_BANK_CONFLICTS,
        BPU_WRITES,
        UOPS_DECODED,
        DECODE_SERIALIZE_CYCLES,
        DECODE_IDLE_SERIALIZE_CYCLES,
        NONSPEC_RESYNC,
        PATCH_MATCH_M_MODE_EXCEPTION,
        PATCH_MATCH_S_MODE_EXCEPTION,
        PATCH_MATCH_U_MODE_EXCEPTION,
        PATCH_MATCH_VS_MODE_EXCEPTION,
        PATCH_MATCH_VU_MODE_EXCEPTION,
        PATCH_MATCH_UCODE,
        PATCH_MATCH_M_MODE_EXCEPTION_CYCLES,
        PATCH_MATCH_S_MODE_EXCEPTION_CYCLES,
        PATCH_MATCH_U_MODE_EXCEPTION_CYCLES,
        PATCH_MATCH_VS_MODE_EXCEPTION_CYCLES,
        PATCH_MATCH_VU_MODE_EXCEPTION_CYCLES,
        PATCH_MATCH_UCODE_CYCLES,
        STALLED_CYCLES_FRONTEND,
        STALLED_CYCLES_BACKEND,
        CYCLES_NO_INT_PRN,
        CYCLES_NO_FP_PRN,
        CYCLES_NO_VEC_PRN,
        CYCLES_NO_VL_PRN,
        CYCLES_NO_VM_PRN,
        CYCLES_NO_ROB,
        CYCLES_DB0_STALL,
        CYCLES_DB1_STALL,
        CYCLES_DB2_STALL,
        CYCLES_DB3_STALL,
        CYCLES_DB4_STALL,
        CYCLES_DB5_STALL,
        CYCLES_DB6_STALL,
        CYCLES_DB7_STALL,
        DISPATCHED_NOPS,
        OP_RETIRED_DIRECT_BRANCH,
        OP_RETIRED_RET_BRANCH,
        OP_RETIRED_INDIRECT_BRANCH,
        OP_RETIRED_COND_BRANCH,
        OP_RETIRED_LD,
        OP_RETIRED_ST,
        OP_RETIRED_INT,
        OP_RETIRED_CSR,
        OP_RETIRED_FP,
        OP_RETIRED_VEC,
        UOP_RETIRED_ANY,
        OP_COMPLETE_LD,
        OP_COMPLETE_ST,
        OP_COMPLETE_INT,
        OP_COMPLETE_FP,
        OP_COMPLETE_VEC,
        OP_ISSUED_PIPE0,
        OP_ISSUED_PIPE1,
        OP_ISSUED_PIPE2,
        OP_ISSUED_PIPE3,
        OP_ISSUED_PIPE4,
        OP_ISSUED_PIPE5,
        OP_ISSUED_PIPE6,
        OP_ISSUED_PIPE7,
        OP_ISSUED_PIPE8,
        OP_ISSUED_PIPE9,
        OP_ISSUED_PIPE10,
        OP_ISSUED_PIPE11,
        OP_ISSUED_PIPE12,
        OP_ISSUED_PIPE13,
        OP_ISSUED_PIPE14,
        OP_ISSUED_PIPE15,
        WASTED_ISSUE_SLOTS_VIA_THROTTLING,
        STORE_UOPS_REJECTED_VIA_STQ_ADVANCE,
        OP_ISSUED_FP64,
        FP64_EXPORT_OVERFLOW,
        CACHE_REFERENCES,
        CACHE_MISSES,
        L1D_READ_ACCESS_NON_CLC,
        L1D_READ_ACCESS_CLC,
        L1D_READ_ACCESS_4KX,
        L1D_READ_ACCESS_ALL,
        L1D_WRITE_ACCESS_NON_CLC,
        L1D_WRITE_ACCESS_CLC,
        L1D_WRITE_ACCESS_4KX,
        L1D_WRITE_ACCESS_ALL,
        L1D_PREFETCH_ACCESS_NON_CLC,
        L1D_PREFETCH_ACCESS_CLC,
        L1D_PREFETCH_ACCESS_ALL,
        L1D_MMU_ACCESS,
        L1D_SNOOP_ACCESS,
        L1D_ACCESS_ALL,
        L1D_READ_MISS,
        L1D_WRITE_MISS,
        L1D_PREFETCH_MISS,
        L1D_MMU_MISS,
        L1D_MISS_ALL,
        TRANSBUF_OR_REQBUF_CANNOT_ALLOC_LOAD,
        TRANSBUF_OR_REQBUF_CANNOT_ALLOC_STORE,
        TRANSBUF_OR_REQBUF_CANNOT_ALLOC_PREFETCH,
        TRANSBUF_OR_REQBUF_CANNOT_ALLOC_MMU,
        TRANSBUF_CANNOT_ALLOC_ALL,
        L1D_WRITE_UPGRADE_REQ,
        DTLB_READ_ACCESS,
        DTLB_WRITE_ACCESS,
        DTLB_PREFETCH_ACCESS,
        DTLB_READ_ACCESS_CACHEABLE,
        DTLB_READ_ACCESS_NONCACHEABLE,
        DTLB_WRITE_ACCESS_CACHEABLE,
        DTLB_WRITE_ACCESS_NONCACHEABLE,
        DTLB_ACCESS_ALL,
        DTLB_READ_MISS,
        DTLB_WRITE_MISS,
        DTLB_PREFETCH_MISS,
        DTLB_MISS_4K,
        DTLB_MISS_64K,
        DTLB_MISS_HUGEPAGE,
        DTLB_MISS_ALL,
        LEAF_TLB_ACCESS_LS,
        LEAF_TLB_ACCESS_FE,
        LEAF_TLB_ACCESS_MMU_PREFETCH,
        LEAF_TLB_ACCESS_ALL,
        LEAF_TLB_MISS_LS,
        LEAF_TLB_MISS_FE,
        LEAF_TLB_MISS_MMU_PREFETCH,
        LEAF_TLB_MISS_ALL,
        NONLEAF_TLB_ACCESS_LS,
        NONLEAF_TLB_ACCESS_FE,
        NONLEAF_TLB_ACCESS_MMU_PREFETCH,
        NONLEAF_TLB_ACCESS_ALL,
        NONLEAF_TLB_MISS_LS,
        NONLEAF_TLB_MISS_FE,
        NONLEAF_TLB_MISS_MMU_PREFETCH,
        NONLEAF_TLB_MISS_ALL,
        PAGE_TABLE_WALKS_LS,
        PAGE_TABLE_WALKS_FE,
        PAGE_TABLE_WALKS_MMU_PREFETCH,
        PAGE_TABLE_WALKS_ALL,
        STLF_REPLAY_LOAD,
        STLF_REPLAY_MMU,
        STLF_REPLAY_ALL,
        DATA_BANK_CONFLICT_REPLAY_LOAD,
        DATA_BANK_CONFLICT_REPLAY_STORE,
        DATA_BANK_CONFLICT_REPLAY_MMU,
        DATA_BANK_CONFLICT_REPLAY_ALL,
        LS_WAY_PREDICTOR_REPLAY_LOAD,
        LS_WAY_PREDICTOR_REPLAY_STORE,
        LS_WAY_PREDICTOR_REPLAY_PREFETCH,
        LS_WAY_PREDICTOR_REPLAY_MMU,
        LS_WAY_PREDICTOR_REPLAY_ALL,
        LS_MICRO_WAY_PREDICTOR_REPLAY_LOAD,
        LS_MICRO_WAY_PREDICTOR_REPLAY_PREFETCH,
        LS_MICRO_WAY_PREDICTOR_REPLAY_ALL,
        TAG_BANK_CONFLICT_REPLAY_LOAD,
        TAG_BANK_CONFLICT_REPLAY_STORE,
        TAG_BANK_CONFLICT_REPLAY_PREFETCH,
        TAG_BANK_CONFLICT_REPLAY_MMU,
        TAG_BANK_CONFLICT_REPLAY_ALL,
        DTLB_REPLAY_LOAD,
        DTLB_REPLAY_STORE,
        DTLB_REPLAY_PREFETCH,
        DTLB_REPLAY_ALL,
        SIPT_REPLAY_LOAD,
        SIPT_REPLAY_STORE,
        SIPT_REPLAY_ALL,
        REQBUF_HIT_REPLAY_LOAD,
        REQBUF_HIT_REPLAY_STORE,
        REQBUF_HIT_REPLAY_MMU,
        REQBUF_HIT_REPLAY_ALL,
        FILLBUF_HIT_REPLAY_LOAD,
        FILLBUF_HIT_REPLAY_STORE,
        FILLBUF_HIT_REPLAY_MMU,
        FILLBUF_HIT_REPLAY_ALL,
        CACHE_HIT_PREDICTOR_REPLAY_LOAD,
        CACHE_HIT_PREDICTOR_REPLAY_STORE,
        CACHE_HIT_PREDICTOR_REPLAY_ALL,
        UTLB_MISS_IN_HIT_MODE,
        UTLB_HIT_PREDICTOR_ACTIVE,
        UTLB_HIT_PREDICTOR_ENTRANCE,
        L1D_MISS_REQBUF_LINK_LOAD,
        L1D_MISS_REQBUF_LINK_STORE,
        L1D_MISS_REQBUF_LINK_MMU,
        L1D_MISS_REQBUF_LINK_ALL,
        L1D_MISS_MISC_REPLAY_LOAD,
        L1D_MISS_MISC_REPLAY_STORE,
        L1D_MISS_MISC_REPLAY_PREFETCH,
        L1D_MISS_MISC_REPLAY_MMU,
        L1D_MISS_MISC_REPLAY_ALL,
        L1D_VICTIM_FILL_EVICT,
        L1D_VICTIM_EARLY_EVICT,
        L1D_VICTIM_DEMAND_REQ,
        L1D_VICTIM_PREFETCH_REQ,
        L1D_VICTIM_MRU_ALLOC,
        L1D_VICTIM_LRU_ALLOC,
        L1D_VICTIM_ALL,
        L1D_CACHE_INVALIDATE_SNOOP,
        L1D_CACHE_INVALIDATE_CMO,
        L1D_CACHE_INVALIDATE_RAS,
        L1D_CACHE_INVALIDATE_ALL,
        LSU_RESYNCS_RAR_STPIPE,
        LSU_RESYNCS_RAR_LDPIPE,
        LSU_RESYNCS_RAR_ALL,
        PFC_PREFETCHES_LATE_L1PEND,
        PFC_PREFETCHES_LATE_REQBUF,
        PFC_PREFETCHES_LATE_WASTED,
        PFC_PREFETCHES_LATE_ALL,
        PFC_AGT_EVICT_CHAINING,
        PFC_PHT_CHAIN_LOOKUP,
        PFC_PHT_CHAIN_HIT,
        PFC_PHT_RTF_TAP_FILTERED,
        PFC_PHT_RTF_CHAIN_FILTERED,
        LS_CHILLOUT_CYCLES_RELAXED,
        LS_CHILLOUT_CYCLES_MEDIUM,
        LS_CHILLOUT_CYCLES_HEAVY,
        LS_CHILLOUT_CYCLES_ALL,
        LS_CHILLOUT_CYCLES_LDC,
        LS_CHILLOUT_CYCLES_STC,
        LS_CHILLOUT_CYCLES_MMU,
        LS_CHILLOUT_CYCLES_CIF,
        LS_CHILLOUT_REQUESTS_LDC,
        LS_CHILLOUT_REQUESTS_STC,
        LS_CHILLOUT_REQUESTS_MMU,
        LS_CHILLOUT_REQUESTS_CIF,
        LS_CHILLOUT_REQUESTS_ALL,
        LS_CHILLOUT_ENTRANCES_LDC,
        LS_CHILLOUT_ENTRANCES_STC,
        LS_CHILLOUT_ENTRANCES_MMU,
        LS_CHILLOUT_ENTRANCES_CIF,
        LS_CHILLOUT_ENTRANCES_ALL,
        UTLB_HIT_LOAD,
        UTLB_HIT_STORE,
        UTLB_HIT_ALL,
        UTLB_MISS_LOAD,
        UTLB_MISS_STORE,
        UTLB_MISS_ALL,
        LDQ_CANNOT_ALLOC,
        MDP_CORRECT_PREDICTION,
        MDP_FALSE_HIT,
        MDP_TOTAL_PREDICTION,
        STALLS_MEM_L1D_MISS,
        STALLS_MEM_L1DTLB_MISS,
        RAR_CANNOT_ALLOC,
        RAW_CANNOT_ALLOC,
        PCB_CANNOT_ALLOC,
        UDB_CANNOT_ALLOC,
        UDB_DATA_RETURN,
        UDB_LOST,
        ATOMICS_RETIRED_LR,
        LR_STALL,
        LD_EXECUTED_VEC_NANO,
        LD_MASKED_VEC_NANO,
        STLF_HITS,
        DFP_ACCESS_LOAD,
        DFP_ACCESS_STORE,
        DFP_ACCESS_MMU,
        DFP_ACCESS_EVICT,
        DFP_ACCESS_FILL,
        DFP_ACCESS_SNOOP,
        DFP_ACCESS_ALL,
        TLB_INVALIDATES,
        STALLS_MEM_STORES,
        LSU_RESYNCS_RAW,
        SMB_WANTS_TO_ALLOC,
        SMB_CANNOT_ALLOC,
        ATOMICS_RETIRED_SC,
        ATOMICS_RETIRED_SC_FAIL,
        ATOMICS_RETIRED_SC_SUCCESS,
        ATOMICS_RETIRED_AMO,
        ST_EXECUTED_VEC_NANO,
        ST_MASKED_VEC_NANO,
        TAP_ACCESS_LOAD,
        TAP_ACCESS_STORE,
        TAP_ACCESS_PREFETCH,
        TAP_ACCESS_MMU,
        TAP_ACCESS_EVICT,
        TAP_ACCESS_FILL,
        TAP_ACCESS_SNOOP,
        TAP_ACCESS_ALL,
        UWP_ACCESS_AGP,
        UWP_ACCESS_ARB,
        UWP_ACCESS_ALL,
        UWP_MISS_AGP,
        UWP_MISS_TAP_DFP,
        UWP_MISS_ALL,
        UWP_TRUE_HIT_AGP,
        UWP_TRUE_HIT_ARB,
        UWP_TRUE_HIT_ALL,
        UWP_INVALIDATE_AGP,
        UWP_INVALIDATE_TAP_DFP,
        UWP_INVALIDATE_ALL,
        WP_ACCESS,
        WP_MISS,
        WP_TRUE_HIT,
        PFC_PREFETCHES_HIT,
        PFC_USELESS_PREFETCHES,
        TLP_ACCESS_LOAD,
        TLP_ACCESS_STORE,
        TLP_ACCESS_PREFETCH,
        TLP_ACCESS_AGP,
        TLP_ACCESS_ARB,
        TLP_ACCESS_HIT_4K,
        TLP_ACCESS_HIT_64K,
        TLP_ACCESS_HIT_HUGEPAGE,
        TLP_ACCESS_ALL,
        FILLBUF_CANNOT_ALLOC,
        LS_ARB_GRANT_CANCEL_LDC,
        LS_ARB_GRANT_CANCEL_STC,
        LS_ARB_GRANT_CANCEL_MMU,
        LS_ARB_GRANT_CANCEL_PFC,
        LS_ARB_GRANT_CANCEL_AGP,
        LS_ARB_GRANT_CANCEL_FILL,
        LS_ARB_GRANT_CANCEL_VICTIM,
        LS_ARB_GRANT_CANCEL_REQUESTOR,
        LS_ARB_GRANT_CANCEL_INTERNAL,
        LS_ARB_GRANT_CANCEL_ALL,
        LS_ARB_ROUND_ROBIN_CYCLES,
        LS_ARB_ROUND_ROBIN_ENTRANCES,
        CACHE_HIT_PREDICTOR_ACTIVE,
        CACHE_HIT_PREDICTOR_ENTRANCE,
        PFC_AGT_CANNOT_ALLOC,
        PFC_AGT_TRAINING_ALLOC,
        PFC_AGT_TRAINING_UPDATE,
        PFC_AGT_TRAINING_TAG_MISS,
        PFC_AGT_TRAINING_PF_HIT,
        PFC_AGT_TRAINING_LOAD,
        PFC_AGT_TRAINING_STORE,
        PFC_AGT_TRAINING_ALL,
        PFC_AGT_EVICT,
        PFC_PHT_TAP_LOOKUP,
        PFC_PHT_TAP_HIT,
        PFC_PHT_AGT_ALLOC,
        PFC_PHT_AGT_UPDATE,
        PFC_PRT_ALLOC,
        PFC_PRT_UPDATE,
        PFC_PRT_CANNOT_ALLOC,
        PFC_NO_TLB_CREDIT_STALLS,
        PFC_NO_TAG_CREDIT_STALLS,
        PFC_PREFETCHES_SENT,
        PFC_PRT_L1D_EVICT_HIT,
        PFC_PRT_REQBUF_ALLOC_HIT,
        SC_HIT_READ,
        SC_HIT_WRITE,
        SC_HIT_PREFETCH,
        SC_MISS_READ,
        SC_MISS_WRITE,
        SC_MISS_PREFETCH,
        LDQ_MISSQ_FULL_DELAY,
        STQ_MISSQ_FULL_DELAY,
        BRANCH_INSTRUCTIONS,
        TB_CYCLES,
        EVENT_COUNT
    } pmc_event_t; 
    //AUTOGENERATED -- END 

    typedef logic [3:0] pmc_counter_t;
    typedef pmc_counter_t [EVENT_COUNT-1:0] pmci_t;

    typedef enum {
        HPMCOUNTER3,
        HPMCOUNTER4,
        HPMCOUNTER5,
        HPMCOUNTER6,
        HPMCOUNTER7,
        HPMCOUNTER8,
        HPMCOUNTER9,
        HPMCOUNTER10 
    } hpm_num_t;

    typedef logic [63:0] hpm_counter_t;
    typedef hpm_counter_t [7:0] hpmi_t;

    // --------------------------------------
    // Pwrmgmt
    // --------------------------------------
    parameter bit PWRMGMT_EN = mods.TOP.PLATFORM.PWRMGMT.PWRMGMT_EN == 1;

    // --------------------------------------
    // typedefs to generate all data types
    // --------------------------------------

    parameter AXI_USER_ID_WIDTH = 1;
    typedef logic [AXI_USER_ID_WIDTH-1:0] user_t;

    `AXI_TYPEDEF_AW_CHAN_T(slv_aw_chan_top, axi_addr_t, axi_id_t, user_t)
    `AXI_TYPEDEF_W_CHAN_T(slv_w_chan_top, axi_data_t, axi_strb_t, user_t)
    `AXI_TYPEDEF_B_CHAN_T(slv_b_chan_top, axi_id_t, user_t)
    `AXI_TYPEDEF_AR_CHAN_T(slv_ar_chan_top, axi_addr_t, axi_id_t, user_t)
    `AXI_TYPEDEF_R_CHAN_T(slv_r_chan_top, axi_data_t, axi_id_t, user_t)
    `AXI_TYPEDEF_REQ_T(slv_req_top, slv_aw_chan_top, slv_w_chan_top, slv_ar_chan_top)
    `AXI_TYPEDEF_RESP_T(slv_resp_top, slv_b_chan_top, slv_r_chan_top)

    `AXI_TYPEDEF_AW_CHAN_T(ncio_slv_aw_chan_top, ncio_axi_addr_t, ncio_axi_id_t, user_t)
    `AXI_TYPEDEF_W_CHAN_T(ncio_slv_w_chan_top, ncio_axi_data_t, ncio_axi_strb_t, user_t)
    `AXI_TYPEDEF_B_CHAN_T(ncio_slv_b_chan_top, ncio_axi_id_t, user_t)
    `AXI_TYPEDEF_AR_CHAN_T(ncio_slv_ar_chan_top, ncio_axi_addr_t, ncio_axi_id_t, user_t)
    `AXI_TYPEDEF_R_CHAN_T(ncio_slv_r_chan_top, ncio_axi_data_t, ncio_axi_id_t, user_t)
    `AXI_TYPEDEF_REQ_T(ncio_slv_req_top, ncio_slv_aw_chan_top, ncio_slv_w_chan_top, ncio_slv_ar_chan_top)
    `AXI_TYPEDEF_RESP_T(ncio_slv_resp_top, ncio_slv_b_chan_top, ncio_slv_r_chan_top)

    `AXI_TYPEDEF_AW_CHAN_T(mst_aw_chan_top, axi_mst_addr_t, axi_mst_id_t, axi_mst_user_t)
    `AXI_TYPEDEF_W_CHAN_T(mst_w_chan_top, axi_mst_data_t, axi_mst_strb_t, axi_mst_user_t)
    `AXI_TYPEDEF_B_CHAN_T(mst_b_chan_top, axi_mst_id_t, axi_mst_user_t)
    `AXI_TYPEDEF_AR_CHAN_T(mst_ar_chan_top, axi_mst_addr_t, axi_mst_id_t, axi_mst_user_t)
    `AXI_TYPEDEF_R_CHAN_T(mst_r_chan_top, axi_mst_data_t, axi_mst_id_t, axi_mst_user_t)
    `AXI_TYPEDEF_REQ_T(mst_req_top, mst_aw_chan_top, mst_w_chan_top, mst_ar_chan_top)
    `AXI_TYPEDEF_RESP_T(mst_resp_top, mst_b_chan_top, mst_r_chan_top)

    `AXI_TYPEDEF_AW_CHAN_T(smc_aw_chan_top, smc_addr_t, smc_id_t, smc_user_t)
    `AXI_TYPEDEF_W_CHAN_T(smc_w_chan_top, smc_data_t, smc_strb_t, smc_user_t)
    `AXI_TYPEDEF_B_CHAN_T(smc_b_chan_top, smc_id_t, smc_user_t)
    `AXI_TYPEDEF_AR_CHAN_T(smc_ar_chan_top, smc_addr_t, smc_id_t, smc_user_t)
    `AXI_TYPEDEF_R_CHAN_T(smc_r_chan_top, smc_data_t, smc_id_t, smc_user_t)
    `AXI_TYPEDEF_REQ_T(smc_req_top, smc_aw_chan_top, smc_w_chan_top, smc_ar_chan_top)
    `AXI_TYPEDEF_RESP_T(smc_resp_top, smc_b_chan_top, smc_r_chan_top)


    // --------------------------------------
    // rv_tester ports
    // --------------------------------------
`define _RV_TESTER_PORTS(input,output)                                                              \
    input  longint unsigned                  clocks,                                                \
    input                                    clk                [rv_tester_params::NCLKS-1:0],      \
    output                                   dut_clk            [rv_tester_params::NCLKS-1:0],      \
    input                                    dut_reset          [rv_tester_params::NCLKS-1:0],      \
    output                                   dut_reset_req,                                         \
    input   logic                            ndmreset_ack,                                          \
    input                                    dut_reset_req_active,                                  \
    input                                    warm_reset_req,                                        \
    input                                    force_ref_clk,                                         \
    output [rv_tester_params::NHARTS-1:0]    core_no_fetch,                                         \
    input  [rv_tester_params::NRESETS-1:0]   reset, /*Packed so zebu can easily force*/             \
    input  [rv_tester_params::NHOLDS-1:0]    reset_hold,                                            \
    input  rv_tester_params::bootstrap_t     bootstrap,                                             \
    input  rv_tester_pkg::nmi_t              nmi                [rv_tester_params::NHARTS-1:0],     \
    output rv_tester_pkg::nmi_t              nmi_pend           [rv_tester_params::NHARTS-1:0],     \
    input  rv_tester_pkg::interrupt_t        interrupt          [rv_tester_params::NHARTS-1:0],     \
    output rv_tester_params::interrupt_pend_t interrupt_pend    [rv_tester_params::NHARTS-1:0],     \
    output logic [63:0]                      mtime,                                                 \
    output rv_tester_params::msi_t           imsic_msi          [rv_tester_params::NHARTS-1:0],     \
    output                                   debug_mode         [rv_tester_params::NHARTS-1:0],     \
    output                                   disable_checks,                                        \
    output                                   dut_terminate,                                         \
    output                                   tj_shutdown,                                           \
    output                                   tj_max,                                                \
    output                                   pll_dfs_done,                                          \
    output                                   pll_shutdown_done,                                     \
    input                                    terminate,                                             \
    input  logic                             terminated,                                            \
    input  logic                             terminate_now,                                            \
    output                                   quiesced,                                              \
    output                                   unconditional_terminate,                               \
    input                                    boot_done_all,                                         \
    input logic [64-1:0]                     cosim_eot_addr,                                        \
    input  rv_tester_pkg::dm_write_t         dmi_write,                                             \
    input  rv_tester_pkg::jtag_if_t          jtag_req,                                              \
    input  rv_tester_pkg::jtag_if_tck        jtag_tck_trst,                                         \
    output  rv_tester_pkg::jtag_if_out       jtag_resp,                                             \
    output                                   dmi_req_ready,                                         \
    output                                   dmi_resp_valid,                                        \
    output rv_tester_pkg::dmi_resp_t         dmi_resp,                                              \
    input                                    dmi_req_valid,                                         \
    input  rv_tester_pkg::dmi_req_t          dmi_req,                                               \
    input                                    dmi_resp_ready,                                        \
    output [7:0]                             DM_DebugReq_Valids,                                  \
    output logic [51:0]                      dfetch_cl_addr[1:0],                                 \
    output logic [1:0]                       dfetch_cl_valid,                                      \
    output logic [51:0]                      writeback_cl_addr[1:0],                               \
    output logic [1:0]                       writeback_cl_valid,                                    \
    output logic [51:0]                      devict_cl_addr [rv_tester_params::NHARTS-1:0],         \
    output logic                             devict_cl_valid [rv_tester_params::NHARTS-1:0],        \
    output logic [51:0]                      flush_cl_addr [rv_tester_params::NHARTS-1:0],         \
    output logic                             flush_cl_valid [rv_tester_params::NHARTS-1:0],        \
    output rv_tester_params::rvfi_t          [rv_tester_params::TOTAL_NRETS-1:0]      rvfi,         \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NREADS-1:0]     mcmi_read,    \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NINSERTS-1:0]   mcmi_insert,  \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NWRITES-1:0]    mcmi_write,   \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NBYPASSES-1:0]  mcmi_bypass,  \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NIFETCHES-1:0]  mcmi_ifetch_req,  \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NIFETCHES-1:0]  mcmi_ifetch_resp,  \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NIEVICTS-1:0]   mcmi_ievict,  \
    output rv_tester_params::csri_t          csri         [rv_tester_params::NHARTS-1:0],           \
    output rv_tester_params::pmci_t          pmci         [rv_tester_params::NHARTS-1:0],           \
    output rv_tester_params::hpmi_t          hpmi         [rv_tester_params::NHARTS-1:0],           \
    output rv_tester_pkg::sc_pmci_t          sc_pmci,                                               \
    output logic                                            dm_mem_tx_vld,                          \
    output logic                                            dm_mem_tx_we,                           \
    output logic [rv_tester_params::DM_AXI_ADDR_WIDTH-1:0]  dm_mem_tx_addr,                         \
    output logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_rd_data,                      \
    output logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_wr_data,                      \
    output logic [rv_tester_params::DM_AXI_STRB_WIDTH-1:0]  dm_mem_tx_wr_data_be,                   \
                                                                                                    \
    output logic                                            dmi_tx_req_vld,                         \
    output logic                                            dmi_tx_resp_vld,                        \
    output rv_tester_pkg::dmi_req_t                         dmi_tx_req,                             \
    output rv_tester_pkg::dmi_resp_t                        dmi_tx_resp,                            \
                                                                                                    \
    output rv_tester_params::slv_req_top     axi_req [rv_tester_params::AXI_TOTAL-1:0],             \
    input  rv_tester_params::slv_resp_top    axi_rsp [rv_tester_params::AXI_TOTAL-1:0],             \
    output rv_tester_params::ncio_slv_req_top     ncio_axi_req [rv_tester_params::NCIO_AXI_TOTAL-1:0],             \
    input  rv_tester_params::ncio_slv_resp_top    ncio_axi_rsp [rv_tester_params::NCIO_AXI_TOTAL-1:0],             \
    input  rv_tester_params::mst_req_top     axi_req_mst [rv_tester_params::AXI_MST_TOTAL-1:0],     \
    output rv_tester_params::mst_resp_top    axi_rsp_mst [rv_tester_params::AXI_MST_TOTAL-1:0],     \
    input  rv_tester_params::smc_req_top    smc_axi_req_mst [rv_tester_params::SMC_AXI_MST_TOTAL-1:0],   \
    output rv_tester_params::smc_resp_top   smc_axi_rsp_mst [rv_tester_params::SMC_AXI_MST_TOTAL-1:0],   \
    output rv_tester_params::ac_cr_sync AcCrSynci  [rv_tester_params::NHARTS-1:0], \
    output logic [63:0] AcCrCtimeCsr  [rv_tester_params::NHARTS-1:0],              \
    output logic AcCrDebugMode[rv_tester_params::NHARTS-1:0],                      \
    output logic AcCrGateClk[rv_tester_params::NHARTS-1:0],                        \
    output rv_tester_params::cr_ac_axi_pkt AcReqPkti,                              \
    output rv_tester_params::cr_ac_axi_pkt AcReqPktRfClki,                         \
    output logic [63:0] AcMtimei,                                                  \
    output logic AcWarmReset,                                                      \
    output logic [8:0]  AcMtipi,                                                   \
    output logic SmcMtipi,                                                         \
    output logic AcChk_pll_interrupts_in,                                          \
    output rv_tester_params::event_trigger_intf_t event_triggers  [rv_tester_params::NHARTS-1:0]

`define _RV_TESTER_STALL_CHECKER_PORTS(input,output)                                                \
    input clk,                                                                                      \
    input reset_n,                                                                                  \
    input rv_tester_params::slv_req_top     axi_req,                                                \
    input rv_tester_params::slv_resp_top    axi_rsp

`define RV_TESTER_VARS(topology)                                                                    \
    logic [63:0]                             clocks;                                                \
    logic                                    clk             [rv_tester_params::NCLKS-1:0];         \
    logic                                    dut_clk         [rv_tester_params::NCLKS-1:0];         \
    logic                                    dut_reset       [rv_tester_params::NCLKS-1:0];         \
    logic                                    dut_reset_req;                                         \
    logic                                    ndmreset_ack;                                          \
    logic                                    dut_reset_req_active;                                  \
    logic                                    warm_reset_req;                                        \
    logic                                    force_ref_clk;                                         \
    logic [rv_tester_params::NHARTS-1:0]     core_no_fetch;                                         \
    logic [rv_tester_params::NRESETS-1:0]    reset           /* Packed so zebu can force easily */; \
    logic [rv_tester_params::NHOLDS-1:0]     reset_hold;                                            \
    rv_tester_params::bootstrap_t            bootstrap;                                             \
    rv_tester_pkg::nmi_t                     nmi             [rv_tester_params::NHARTS-1:0];        \
    rv_tester_pkg::nmi_t                     nmi_pend        [rv_tester_params::NHARTS-1:0];        \
    rv_tester_pkg::interrupt_t               interrupt       [rv_tester_params::NHARTS-1:0];        \
    rv_tester_params::interrupt_pend_t       interrupt_pend  [rv_tester_params::NHARTS-1:0];        \
    logic [63:0]                             mtime;                                                 \
    rv_tester_params::msi_t                  imsic_msi       [rv_tester_params::NHARTS-1:0];        \
    logic                                    debug_mode      [rv_tester_params::NHARTS-1:0];        \
    logic                                    disable_checks;                                        \
    logic                                    dut_terminate;                                         \
    logic                                    tj_shutdown;                                           \
    logic                                    tj_max;                                                \
    logic                                    pll_dfs_done;                                          \
    logic                                    pll_shutdown_done;                                     \
    logic                                    terminate;                                             \
    logic                                    terminated;                                            \
    logic                                    terminate_now;                                            \
    logic                                    quiesced;                                              \
    logic                                    unconditional_terminate;                               \
    logic                                    boot_done_all;                                         \
    logic [64-1:0]                           cosim_eot_addr;                                        \
    rv_tester_pkg::dm_write_t                dmi_write;                                             \
    rv_tester_pkg::jtag_if_t                 jtag_req;                                              \
    rv_tester_pkg::jtag_if_tck               jtag_tck_trst;                                         \
    rv_tester_pkg::jtag_if_out               jtag_resp;                                             \
    logic                                    dmi_req_ready;                                         \
    logic                                    dmi_resp_valid;                                        \
    rv_tester_pkg::dmi_resp_t                dmi_resp;                                              \
    logic                                    dmi_req_valid;                                         \
    rv_tester_pkg::dmi_req_t                 dmi_req;                                               \
    logic                                    dmi_resp_ready;                                        \
    logic [7:0]     DM_DebugReq_Valids;                                  \
                                                                                                    \
    logic                                            dm_mem_tx_vld;                                 \
    logic                                            dm_mem_tx_we;                                  \
    logic [rv_tester_params::DM_AXI_ADDR_WIDTH-1:0]  dm_mem_tx_addr;                                \
    logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_rd_data;                             \
    logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_wr_data;                             \
    logic [rv_tester_params::DM_AXI_STRB_WIDTH-1:0]  dm_mem_tx_wr_data_be;                          \
                                                                                                    \
    logic                                            dmi_tx_req_vld;                                \
    logic                                            dmi_tx_resp_vld;                               \
    rv_tester_pkg::dmi_req_t                         dmi_tx_req;                                    \
    rv_tester_pkg::dmi_resp_t                        dmi_tx_resp;                                   \
                                                                                                    \
    logic [51:0]                             dfetch_cl_addr[1:0];                                        \
    logic [1:0]                              dfetch_cl_valid;                                       \
    logic [51:0]                             writeback_cl_addr[1:0];                                     \
    logic [1:0]                              writeback_cl_valid;                                    \
    logic [51:0]                             devict_cl_addr [rv_tester_params::NHARTS-1:0];         \
    logic                                    devict_cl_valid [rv_tester_params::NHARTS-1:0];        \
    logic [51:0]                             flush_cl_addr  [rv_tester_params::NHARTS-1:0];         \
    logic                                    flush_cl_valid [rv_tester_params::NHARTS-1:0];        \
    rv_tester_params::rvfi_t                 [rv_tester_params::TOTAL_NRETS-1:0]       rvfi;        \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NREADS-1:0]      mcmi_read;   \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NINSERTS-1:0]    mcmi_insert; \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NWRITES-1:0]     mcmi_write;  \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NBYPASSES-1:0]   mcmi_bypass; \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NIFETCHES-1:0]   mcmi_ifetch_req; \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NIFETCHES-1:0]   mcmi_ifetch_resp; \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NIEVICTS-1:0]    mcmi_ievict; \
    rv_tester_params::csri_t                 csri          [rv_tester_params::NHARTS-1:0];          \
    rv_tester_params::pmci_t                 pmci          [rv_tester_params::NHARTS-1:0];          \
    rv_tester_params::hpmi_t                 hpmi          [rv_tester_params::NHARTS-1:0];          \
    rv_tester_pkg::sc_pmci_t                 sc_pmci;                                               \
    rv_tester_params::slv_req_top            axi_req [rv_tester_params::AXI_TOTAL-1:0];             \
    rv_tester_params::slv_resp_top           axi_rsp [rv_tester_params::AXI_TOTAL-1:0];             \
    rv_tester_params::ncio_slv_req_top       ncio_axi_req [rv_tester_params::NCIO_AXI_TOTAL-1:0];   \
    rv_tester_params::ncio_slv_resp_top      ncio_axi_rsp [rv_tester_params::NCIO_AXI_TOTAL-1:0];   \
    rv_tester_params::mst_req_top            axi_req_mst [rv_tester_params::AXI_MST_TOTAL-1:0];     \
    rv_tester_params::mst_resp_top           axi_rsp_mst [rv_tester_params::AXI_MST_TOTAL-1:0];     \
    rv_tester_params::mst_req_top            axi_msi;                                               \
    rv_tester_params::mst_req_top            [rv_tester_params::NHARTS-1:0] axi_msi_packets  ;      \
    rv_tester_params::mst_req_top            [rv_tester_params::NHARTS-1:0] axi_ipi_packets  ;      \
    rv_tester_params::smc_req_top      smc_axi_req_mst  [rv_tester_params::SMC_AXI_MST_TOTAL-1:0];  \
    rv_tester_params::smc_resp_top     smc_axi_rsp_mst  [rv_tester_params::SMC_AXI_MST_TOTAL-1:0];  \
    rv_tester_params::ac_cr_sync AcCrSynci [rv_tester_params::NHARTS-1:0];                          \
    logic [63:0] AcCrCtimeCsr  [rv_tester_params::NHARTS-1:0];                                      \
    logic AcCrDebugMode[rv_tester_params::NHARTS-1:0];                                              \
    logic AcCrGateClk[rv_tester_params::NHARTS-1:0];                                                \
    rv_tester_params::cr_ac_axi_pkt AcReqPkti;                                                      \
    rv_tester_params::cr_ac_axi_pkt AcReqPktRfClki;                                                 \
    logic [63:0] AcMtimei;                                                                          \
    logic AcWarmReset;                                                                              \
    logic [8:0]  AcMtipi;                                                                           \
    logic SmcMtipi;                                                                                 \
    logic AcChk_pll_interrupts_in;                                                                  \
    rv_tester_params::event_trigger_intf_t event_triggers [rv_tester_params::NHARTS-1:0];

`define RV_TESTER_PORTS `_RV_TESTER_PORTS(input,output)

endpackage
