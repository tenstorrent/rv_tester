///////////////includes///////////////////////////////

`include "axi/typedef.svh"

/////////////////////////////////////////////////////

package rv_tester_params;

    import topology_pkg::mods;

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
    // AXI interface
    // --------------------------------------
    parameter AXI_TOTAL = mods.TOP.PLATFORM.AXI.TOTAL;
    parameter AXI_ADDR_WIDTH = mods.TOP.PLATFORM.AXI.ADDR_WIDTH;
    parameter AXI_DATA_WIDTH = mods.TOP.PLATFORM.AXI.DATA_WIDTH;
    parameter AXI_STRB_WIDTH = mods.TOP.PLATFORM.AXI.STRB_WIDTH;
    parameter AXI_ID_WIDTH = mods.TOP.PLATFORM.AXI.ID_WIDTH;
    //NCIO
    parameter NCIO_AXI_TOTAL = mods.TOP.PLATFORM.NCIO_AXI.TOTAL;
    parameter NCIO_AXI_ADDR_WIDTH = mods.TOP.PLATFORM.NCIO_AXI.ADDR_WIDTH;
    parameter NCIO_AXI_DATA_WIDTH = mods.TOP.PLATFORM.NCIO_AXI.DATA_WIDTH;
    parameter NCIO_AXI_STRB_WIDTH = mods.TOP.PLATFORM.NCIO_AXI.STRB_WIDTH;
    parameter NCIO_AXI_ID_WIDTH = mods.TOP.PLATFORM.NCIO_AXI.ID_WIDTH;
    //
    parameter AXI_MST_TOTAL = mods.TOP.PLATFORM.AXI_MST.TOTAL;
    parameter AXI_MST_ADDR_WIDTH = mods.TOP.PLATFORM.AXI_MST.ADDR_WIDTH;
    parameter AXI_MST_DATA_WIDTH = mods.TOP.PLATFORM.AXI_MST.DATA_WIDTH;
    parameter AXI_MST_STRB_WIDTH = mods.TOP.PLATFORM.AXI_MST.STRB_WIDTH;
    parameter AXI_MST_ID_WIDTH = mods.TOP.PLATFORM.AXI_MST.ID_WIDTH;
    parameter AXI_USER_ID_WIDTH = 1;
    parameter AXI_MST2_TOTAL = mods.TOP.PLATFORM.AXI_MST2.TOTAL;
    parameter AXI_MST2_ADDR_WIDTH = mods.TOP.PLATFORM.AXI_MST2.ADDR_WIDTH;
    parameter AXI_MST2_DATA_WIDTH = mods.TOP.PLATFORM.AXI_MST2.DATA_WIDTH;
    parameter AXI_MST2_STRB_WIDTH = mods.TOP.PLATFORM.AXI_MST2.STRB_WIDTH;
    parameter AXI_MST2_ID_WIDTH = mods.TOP.PLATFORM.AXI_MST2.ID_WIDTH;

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

    typedef logic [AXI_MST2_ADDR_WIDTH-1:0] axi_mst2_addr_t;
    typedef logic [AXI_MST2_DATA_WIDTH-1:0] axi_mst2_data_t;
    typedef logic [AXI_MST2_STRB_WIDTH-1:0] axi_mst2_strb_t;
    typedef logic [AXI_MST2_ID_WIDTH  -1:0] axi_mst2_id_t;

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
        axi_mst2_id_t                ar_id    ;
        axi_mst2_addr_t              ar_addr  ;
        axi_len_t                   ar_len   ;
        axi_size_t                  ar_size  ;
        axi_burst_t                 ar_burst ;
        logic                       ar_lock  ;

        logic                       aw_valid ;
        axi_mst2_id_t                aw_id    ;
        axi_mst2_addr_t              aw_addr  ;
        axi_len_t                   aw_len   ;
        axi_size_t                  aw_size  ;
        axi_burst_t                 aw_burst ;
        logic                       aw_lock  ;
        axi_atop_t                  aw_atop  ;

        logic                       w_valid  ;
        axi_mst2_data_t              w_data   ;
        axi_mst2_strb_t              w_strb   ;
        logic                       w_last   ;

        logic                       b_ready  ;
        logic                       r_ready  ;
    } axi_req_mst2_t;

    typedef struct packed {
        logic                       b_valid  ;
        axi_mst2_id_t                b_id     ;
        axi_resp_t                  b_resp   ;

        logic                       r_valid  ;
        axi_mst2_id_t                r_id     ;
        axi_mst2_data_t              r_data   ;
        axi_resp_t                  r_resp   ;
        logic                       r_last   ;

        logic                       aw_ready ;
        logic                       ar_ready ;
        logic                       w_ready  ;
    } axi_rsp_mst2_t;

    // --------------------------------------
    // Bootstrap
    // --------------------------------------
    typedef struct packed {
        logic [VALEN-1:0]           boot_addr;
    } bootstrap_t;

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
        logic [ILEN-1:0]            insn     ;
        logic                       trap     ;
        logic [XLEN-1:0]            cause    ;
        logic                       halt     ;
        logic                       intr     ;
        logic [4-1:0]               mode     ;
        logic                       vec_cracked;
        logic [2-1:0]               ixl      ;
        logic [5-1:0]               rd_addr  ;
        logic [XLEN-1:0]            rd_wdata ;
        logic                       frd_valid;
        logic [5-1:0]               frd_addr ;
        logic [FLEN-1:0]            frd_wdata;
        logic                       vrd_valid;
        logic [5-1:0]               vrd_addr ;
        logic [VLEN-1:0]            vrd_wdata;
        logic                       csr_valid;
        logic [CSRLEN-1:0]          csr_addr ;
        logic [XLEN-1:0]            csr_wdata;
        logic [XLEN-1:0]            csr_wmask;
        logic [VALEN-1:0]           pc_rdata ;
        logic [VALEN-1:0]           pc_wdata ;
        logic [VALEN-1:0]           mem_addr ;
        logic [PALEN-1:0]           mem_paddr;
        logic [(RDLEN/8)-1:0]       mem_rmask;
        logic [(RDLEN/8)-1:0]       mem_wmask;
        logic [RDLEN-1:0]           mem_rdata;
        logic [RDLEN-1:0]           mem_wdata;
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
    parameter bit [NHARTS-1:0][31:0] NREADS = mods.TOP.PLATFORM.COSIM.MCMI.NREADS;
    parameter bit [NHARTS-1:0][31:0] NINSERTS = mods.TOP.PLATFORM.COSIM.MCMI.NINSERTS;
    parameter bit [NHARTS-1:0][31:0] NWRITES = mods.TOP.PLATFORM.COSIM.MCMI.NWRITES;
    parameter bit [NHARTS-1:0][31:0] NBYPASSES = mods.TOP.PLATFORM.COSIM.MCMI.NBYPASSES;
    parameter bit [NHARTS-1:0][31:0] NREADS_CUMSUM   = mods.TOP.PLATFORM.COSIM.MCMI.NREADS_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NINSERTS_CUMSUM = mods.TOP.PLATFORM.COSIM.MCMI.NINSERTS_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NWRITES_CUMSUM  = mods.TOP.PLATFORM.COSIM.MCMI.NWRITES_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NBYPASSES_CUMSUM  = mods.TOP.PLATFORM.COSIM.MCMI.NBYPASSES_CUMSUM;

    typedef struct packed {
        logic                       valid;
        logic [15:0]                hart ;
        logic [64-1:0]              cycle;
        logic [64-1:0]              order;
        logic [PALEN-1:0]           addr ;
        logic [(CLLEN/8)-1:0]       mask ;
        logic [CLLEN-1:0]           data ;
    } mcmi_t;

    // --------------------------------------
    // CSRI - Control Status Registers
    // --------------------------------------
    typedef enum {
        FFLAGS,
        FRM,
        FCSR,
        VSTART,
        VXSAT,
        VXRM,
        VCSR,
        SSTATUS,
        STVEC,
        SCOUNTEREN,
        SSCRATCH,
        SEPC,
        SCAUSE,
        STVAL,
        SIP,
        SATP,
        VSSTATUS,
        VSEPC,
        VSCAUSE,
        VSTVAL,
        VSATP,
        MSTATUS,
        MEDELEG,
        MIDELEG,
        MTVEC,
        MCOUNTEREN,
        MVIEN,
        MVIP,
        MHPMEVENT3,
        MHPMEVENT4,
        MHPMEVENT5,
        MHPMEVENT6,
        MHPMEVENT7,
        MHPMEVENT8,
        MHPMEVENT9,
        MHPMEVENT10,
        MHPMEVENT11,
        MHPMEVENT12,
        MHPMEVENT13,
        MHPMEVENT14,
        MHPMEVENT15,
        MHPMEVENT16,
        MHPMEVENT17,
        MHPMEVENT18,
        MHPMEVENT19,
        MHPMEVENT20,
        MHPMEVENT21,
        MHPMEVENT22,
        MHPMEVENT23,
        MHPMEVENT24,
        MHPMEVENT25,
        MHPMEVENT26,
        MHPMEVENT27,
        MHPMEVENT28,
        MHPMEVENT29,
        MHPMEVENT30,
        MHPMEVENT31,
        MSCRATCH,
        MEPC,
        MCAUSE,
        MTVAL,
        MIP,
        MTINST,
        MTVAL2,
        SCONTEXT,
        CSATPSPEC,
        CXTVALSPEC,
        HSTATUS,
        HIDELEG,
        HTVAL,
        HTINST,
        MSECCFG,
        TSELECT,
        TDATA1,
        TDATA2,
        TDATA3,
        TINFO,
        TCONTROL,
        MCONTEXT,
        DCSR,
        DPC,
        DSCRATCH0,
        DSCRATCH1,
        CDTVEC,
        CPRIV,
        VL_CSR,
        VTYPE_CSR,
        VLENB,
        STOPI,
        VSTOPI,
        MVENDORID,
        MARCHID,
        MIMPID,
        MHARTID,
        MCONFIGPTR,
        MTOPI,
        CSR_COUNT
    } csr_list_t;
    typedef struct packed {
        logic                       valid;
        logic [CSRLEN-1:0]          addr;
        logic [XLEN-1:0]            data;
        logic [XLEN-1:0]            mask;
    } csr_entry_t;

    typedef csr_entry_t [CSR_COUNT-1:0] csri_t;

    // --------------------------------------
    // PMCI - Performance Monitoring Counters
    // --------------------------------------
    parameter bit PMCI_EN = mods.TOP.PLATFORM.PMCI.ENABLE == 1;
    parameter bit [NHARTS-1:0][31:0] LS_PIPES = mods.TOP.PLATFORM.PMCI.LS_PIPES;

    //AUTOGENERATED -- NOTOUCH
    typedef enum {
        CPU_CYCLES,
        INSTRUCTIONS,
        CACHE_REFERENCES,
        CACHE_MISSES,
        BRANCH_INSTRUCTIONS,
        BRANCH_MISSES,
        BUS_CYCLES,
        STALLED_CYCLES_FRONTEND,
        STALLED_CYCLES_BACKEND,
        REF_CPU_CYCLES,
        L1D_READ_ACCESS,
        L1D_READ_MISS,
        L1D_WRITE_ACCESS,
        L1D_WRITE_MISS,
        L1D_PREFETCH_ACCESS,
        L1D_PREFETCH_MISS,
        L1I_READ_ACCESS,
        L1I_READ_MISS,
        L1I_WRITE_ACCESS,
        L1I_WRITE_MISS,
        L1I_PREFETCH_ACCESS,
        L1I_PREFETCH_MISS,
        LL_READ_ACCESS,
        LL_READ_MISS,
        LL_WRITE_ACCESS,
        LL_WRITE_MISS,
        LL_PREFETCH_ACCESS,
        LL_PREFETCH_MISS,
        DTLB_READ_ACCESS,
        DTLB_READ_MISS,
        DTLB_WRITE_ACCESS,
        DTLB_WRITE_MISS,
        DTLB_PREFETCH_ACCESS,
        DTLB_PREFETCH_MISS,
        ITLB_READ_ACCESS,
        ITLB_READ_MISS,
        ITLB_WRITE_ACCESS,
        ITLB_WRITE_MISS,
        ITLB_PREFETCH_ACCESS,
        ITLB_PREFETCH_MISS,
        BPU_WRITE_ACCESS,
        L1D_CACHE_INVALIDATE,
        STALLS_MEM_L1D_MISS,
        STALLS_MEM_STORES,
        STALLS_MEM_L1DTLB_MISS,
        STALLS_MEM_L1D_MISS_L2_MISS,
        STALLS_MEM_ANY,
        STALLS_MEM_L1I_MISS,
        NFP_MISPREDICT,
        BR_IMMED_SPEC,
        BR_INDIRECT_SPEC,
        BR_RET_SPEC,
        LD_SPEC,
        ST_SPEC,
        INT_SPEC,
        FP_SPEC,
        UOP_ISSUED,
        TOTAL_UOPS_FLUSHED,
        TOTAL_IND_BR_RETIRED,
        TOTAL_IND_BR_RETIRED_MISPRED,
        LSU_RESYNCS,
        TOTAL_FLUSHES,
        LOAD_MISAL_ACCESSES,
        STORE_MISAL_ACCESSES,
        STLF_HITS,
        VECTOR_BUSY_CYCLES,
        EVENT_COUNT
    } pmc_event_t;

    typedef logic [3:0] pmc_counter_t;
    typedef pmc_counter_t [EVENT_COUNT-1:0] pmci_t;

    // --------------------------------------
    // typedefs to generate all data types
    // --------------------------------------
    
    typedef logic [AXI_USER_ID_WIDTH-1:0] user_t;

    `AXI_TYPEDEF_AW_CHAN_T(mst_aw_chan_top, axi_mst_addr_t, axi_mst_id_t, user_t)
    `AXI_TYPEDEF_AW_CHAN_T(slv_aw_chan_top, axi_addr_t, axi_id_t, user_t)
    `AXI_TYPEDEF_W_CHAN_T(mst_w_chan_top, axi_mst_data_t, axi_mst_strb_t, user_t)
    `AXI_TYPEDEF_W_CHAN_T(slv_w_chan_top, axi_data_t, axi_strb_t, user_t)
    `AXI_TYPEDEF_B_CHAN_T(mst_b_chan_top, axi_mst_id_t, user_t)
    `AXI_TYPEDEF_B_CHAN_T(slv_b_chan_top, axi_id_t, user_t)
    `AXI_TYPEDEF_AR_CHAN_T(mst_ar_chan_top, axi_mst_addr_t, axi_mst_id_t, user_t)
    `AXI_TYPEDEF_AR_CHAN_T(slv_ar_chan_top, axi_addr_t, axi_id_t, user_t)
    `AXI_TYPEDEF_R_CHAN_T(mst_r_chan_top, axi_mst_data_t, axi_mst_id_t, user_t)
    `AXI_TYPEDEF_R_CHAN_T(slv_r_chan_top, axi_data_t, axi_id_t, user_t)
    `AXI_TYPEDEF_REQ_T(mst_req_top, mst_aw_chan_top, mst_w_chan_top, mst_ar_chan_top)
    `AXI_TYPEDEF_REQ_T(slv_req_top, slv_aw_chan_top, slv_w_chan_top, slv_ar_chan_top)
    `AXI_TYPEDEF_RESP_T(mst_resp_top, mst_b_chan_top, mst_r_chan_top)
    `AXI_TYPEDEF_RESP_T(slv_resp_top, slv_b_chan_top, slv_r_chan_top)
     //NCIO
    `AXI_TYPEDEF_AW_CHAN_T(ncio_slv_aw_chan_top, ncio_axi_addr_t, ncio_axi_id_t, user_t)
    `AXI_TYPEDEF_W_CHAN_T(ncio_slv_w_chan_top, ncio_axi_data_t, ncio_axi_strb_t, user_t)
    `AXI_TYPEDEF_B_CHAN_T(ncio_slv_b_chan_top, ncio_axi_id_t, user_t)
    `AXI_TYPEDEF_AR_CHAN_T(ncio_slv_ar_chan_top, ncio_axi_addr_t, ncio_axi_id_t, user_t)
    `AXI_TYPEDEF_R_CHAN_T(ncio_slv_r_chan_top, ncio_axi_data_t, ncio_axi_id_t, user_t)
    `AXI_TYPEDEF_REQ_T(ncio_slv_req_top, ncio_slv_aw_chan_top, ncio_slv_w_chan_top, ncio_slv_ar_chan_top)
    `AXI_TYPEDEF_RESP_T(ncio_slv_resp_top, ncio_slv_b_chan_top, ncio_slv_r_chan_top)
     //
    `AXI_TYPEDEF_AW_CHAN_T(mst2_aw_chan_top, axi_mst2_addr_t, axi_mst2_id_t, user_t)
    `AXI_TYPEDEF_W_CHAN_T(mst2_w_chan_top, axi_mst2_data_t, axi_mst2_strb_t, user_t)
    `AXI_TYPEDEF_B_CHAN_T(mst2_b_chan_top, axi_mst2_id_t, user_t)
    `AXI_TYPEDEF_AR_CHAN_T(mst2_ar_chan_top, axi_mst2_addr_t, axi_mst2_id_t, user_t)
    `AXI_TYPEDEF_R_CHAN_T(mst2_r_chan_top, axi_mst2_data_t, axi_mst2_id_t, user_t)
    `AXI_TYPEDEF_REQ_T(mst2_req_top, mst2_aw_chan_top, mst2_w_chan_top, mst2_ar_chan_top)
    `AXI_TYPEDEF_RESP_T(mst2_resp_top, mst2_b_chan_top, mst2_r_chan_top)

    // --------------------------------------
    // rv_tester ports
    // --------------------------------------
`define _RV_TESTER_PORTS(input,output)                                                              \
    input                                    clk,                                                   \
    input                                    reset,                                                 \
    input  rv_tester_params::bootstrap_t     bootstrap,                                             \
    input  rv_tester_pkg::interrupt_t        interrupt          [rv_tester_params::NHARTS-1:0],     \
    output rv_tester_pkg::interrupt_t        interrupt_pend     [rv_tester_params::NHARTS-1:0],     \
    output                                   debug_mode         [rv_tester_params::NHARTS-1:0],     \
    input                                    terminate,                                             \
    input  logic                             terminated,                                            \
    output                                   quiesced,                                              \
                                                                                                    \
    input  rv_tester_pkg::dm_write_t         dmi_write,                                             \
    output                                   dmi_req_ready,                                         \
    output                                   dmi_resp_valid,                                        \
    output rv_tester_pkg::dmi_resp_t         dmi_resp,                                              \
    input                                    dmi_req_valid,                                         \
    input  rv_tester_pkg::dmi_req_t          dmi_req,                                               \
    input                                    dmi_resp_ready,                                        \
                                                                                                    \
    output rv_tester_params::rvfi_t          [rv_tester_params::TOTAL_NRETS-1:0]      rvfi,         \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NREADS-1:0]     mcmi_read,    \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NINSERTS-1:0]   mcmi_insert,  \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NWRITES-1:0]    mcmi_write,   \
    output rv_tester_params::mcmi_t          [rv_tester_params::TOTAL_NBYPASSES-1:0]  mcmi_bypass,  \
    output rv_tester_params::csri_t          csri         [rv_tester_params::NHARTS-1:0],           \
    output rv_tester_params::pmci_t          pmci         [rv_tester_params::NHARTS-1:0],           \
                                                                                                    \
    output logic                                            dm_mem_tx_vld,                          \
    output logic                                            dm_mem_tx_we,                           \
    output logic [rv_tester_params::DM_AXI_ADDR_WIDTH-1:0]  dm_mem_tx_addr,                         \
    output logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_rd_data,                      \
    output logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_wr_data,                      \
    output logic [rv_tester_params::DM_AXI_STRB_WIDTH-1:0]  dm_mem_tx_wr_data_be,                   \
                                                                                                    \
    output rv_tester_params::slv_req_top     axi_req [rv_tester_params::AXI_TOTAL-1:0],             \
    input  rv_tester_params::slv_resp_top    axi_rsp [rv_tester_params::AXI_TOTAL-1:0],             \
    output rv_tester_params::ncio_slv_req_top     ncio_axi_req [rv_tester_params::NCIO_AXI_TOTAL-1:0],             \
    input  rv_tester_params::ncio_slv_resp_top    ncio_axi_rsp [rv_tester_params::NCIO_AXI_TOTAL-1:0],             \
    input  rv_tester_params::mst_req_top     axi_req_mst [rv_tester_params::AXI_MST_TOTAL-1:0],     \
    output rv_tester_params::mst_resp_top    axi_rsp_mst [rv_tester_params::AXI_MST_TOTAL-1:0],     \
    input  rv_tester_params::mst2_req_top    axi_req_mst2 [rv_tester_params::AXI_MST2_TOTAL-1:0],   \
    output rv_tester_params::mst2_resp_top   axi_rsp_mst2 [rv_tester_params::AXI_MST2_TOTAL-1:0]


`define RV_TESTER_VARS(topology)                                                                    \
    logic                                    clk;                                                   \
    logic                                    reset;                                                 \
    rv_tester_params::bootstrap_t            bootstrap;                                             \
    rv_tester_pkg::interrupt_t               interrupt       [rv_tester_params::NHARTS-1:0];        \
    rv_tester_pkg::interrupt_t               interrupt_pend  [rv_tester_params::NHARTS-1:0];        \
    logic                                    debug_mode      [rv_tester_params::NHARTS-1:0];        \
    logic                                    terminate;                                             \
    logic                                    terminated;                                            \
    logic                                    quiesced;                                              \
    rv_tester_pkg::dm_write_t                dmi_write;                                             \
    logic                                    dmi_req_ready;                                         \
    logic                                    dmi_resp_valid;                                        \
    rv_tester_pkg::dmi_resp_t                dmi_resp;                                              \
    logic                                    dmi_req_valid;                                         \
    rv_tester_pkg::dmi_req_t                 dmi_req;                                               \
    logic                                    dmi_resp_ready;                                        \
                                                                                                    \
    logic                                            dm_mem_tx_vld;                                 \
    logic                                            dm_mem_tx_we;                                  \
    logic [rv_tester_params::DM_AXI_ADDR_WIDTH-1:0]  dm_mem_tx_addr;                                \
    logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_rd_data;                             \
    logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_wr_data;                             \
    logic [rv_tester_params::DM_AXI_STRB_WIDTH-1:0]  dm_mem_tx_wr_data_be;                          \
                                                                                                    \
    rv_tester_params::rvfi_t                 [rv_tester_params::TOTAL_NRETS-1:0]       rvfi;        \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NREADS-1:0]      mcmi_read;   \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NINSERTS-1:0]    mcmi_insert; \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NWRITES-1:0]     mcmi_write;  \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NBYPASSES-1:0]   mcmi_bypass; \
    rv_tester_params::csri_t                 csri          [rv_tester_params::NHARTS-1:0];          \
    rv_tester_params::pmci_t                 pmci          [rv_tester_params::NHARTS-1:0];          \
    rv_tester_params::slv_req_top            axi_req [rv_tester_params::AXI_TOTAL-1:0];             \
    rv_tester_params::slv_resp_top           axi_rsp [rv_tester_params::AXI_TOTAL-1:0];             \
    rv_tester_params::ncio_slv_req_top       ncio_axi_req [rv_tester_params::NCIO_AXI_TOTAL-1:0];             \
    rv_tester_params::ncio_slv_resp_top      ncio_axi_rsp [rv_tester_params::NCIO_AXI_TOTAL-1:0];             \
    rv_tester_params::mst_req_top            axi_req_mst [rv_tester_params::AXI_MST_TOTAL-1:0];     \
    rv_tester_params::mst_resp_top           axi_rsp_mst [rv_tester_params::AXI_MST_TOTAL-1:0];     \
    rv_tester_params::mst2_req_top           axi_req_mst2  [rv_tester_params::AXI_MST2_TOTAL-1:0];  \
    rv_tester_params::mst2_resp_top          axi_rsp_mst2  [rv_tester_params::AXI_MST2_TOTAL-1:0];

`define RV_TESTER_PORTS `_RV_TESTER_PORTS(input,output)

endpackage
