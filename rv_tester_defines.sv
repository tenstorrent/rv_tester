package rv_tester_params;

    import topology_pkg::mods;

    // --------------------------------------
    // Platform RISCV Arch
    // --------------------------------------
    parameter NHARTS = mods.TOP.PLATFORM.NHARTS;
    parameter HARTLEN = NHARTS == 1 ? 1 : $clog2(NHARTS);
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

    // --------------------------------------
    // AXI interface
    // --------------------------------------
    parameter AXI_TOTAL = mods.TOP.PLATFORM.AXI.TOTAL;
    parameter AXI_ADDR_WIDTH = mods.TOP.PLATFORM.AXI.ADDR_WIDTH;
    parameter AXI_DATA_WIDTH = mods.TOP.PLATFORM.AXI.DATA_WIDTH;
    parameter AXI_STRB_WIDTH = mods.TOP.PLATFORM.AXI.STRB_WIDTH;
    parameter AXI_ID_WIDTH = mods.TOP.PLATFORM.AXI.ID_WIDTH;
    parameter AXI_MST_TOTAL = mods.TOP.PLATFORM.AXI_MST.TOTAL;
    parameter AXI_MST_ADDR_WIDTH = mods.TOP.PLATFORM.AXI_MST.ADDR_WIDTH;
    parameter AXI_MST_DATA_WIDTH = mods.TOP.PLATFORM.AXI_MST.DATA_WIDTH;
    parameter AXI_MST_STRB_WIDTH = mods.TOP.PLATFORM.AXI_MST.STRB_WIDTH;
    parameter AXI_MST_ID_WIDTH = mods.TOP.PLATFORM.AXI_MST.ID_WIDTH;

    typedef logic [AXI_ADDR_WIDTH-1:0] axi_addr_t;
    typedef logic [AXI_DATA_WIDTH-1:0] axi_data_t;
    typedef logic [AXI_STRB_WIDTH-1:0] axi_strb_t;
    typedef logic [AXI_ID_WIDTH  -1:0] axi_id_t;

    typedef logic [AXI_MST_ADDR_WIDTH-1:0] axi_mst_addr_t;
    typedef logic [AXI_MST_DATA_WIDTH-1:0] axi_mst_data_t;
    typedef logic [AXI_MST_STRB_WIDTH-1:0] axi_mst_strb_t;
    typedef logic [AXI_MST_ID_WIDTH  -1:0] axi_mst_id_t;

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
    parameter bit [NHARTS-1:0][31:0] NREADS_CUMSUM   = mods.TOP.PLATFORM.COSIM.MCMI.NREADS_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NINSERTS_CUMSUM = mods.TOP.PLATFORM.COSIM.MCMI.NINSERTS_CUMSUM;
    parameter bit [NHARTS-1:0][31:0] NWRITES_CUMSUM  = mods.TOP.PLATFORM.COSIM.MCMI.NWRITES_CUMSUM;

    typedef struct packed {
        logic                       valid    ;
        logic [HARTLEN-1:0]         hart     ;
        logic [64-1:0]              order    ;
        logic [ILEN-1:0]            insn     ;
        logic                       trap     ;
        logic [XLEN-1:0]            cause    ;
        logic                       halt     ;
        logic                       intr     ;
        logic [2-1:0]               mode     ;
        logic [2-1:0]               ixl      ;
        logic [5-1:0]               rd_addr  ;
        logic [XLEN-1:0]            rd_wdata ;
        logic                       frd_valid;
        logic [5-1:0]               frd_addr ;
        logic [FLEN-1:0]            frd_wdata;
        logic                       vrd_valid;
        logic [5-1:0]               vrd_addr ;
        logic [VLEN-1:0]            vrd_wdata;
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
    } rvfi_t;

    // --------------------------------------
    // MCMI - Memory Consistency Model Checks
    // --------------------------------------
    parameter bit MCMI_EN = mods.TOP.PLATFORM.COSIM.MCMI.ENABLE == 1;
    parameter TOTAL_NREADS = MCMI_EN ? mods.TOP.PLATFORM.COSIM.MCMI.TOTAL_NREADS : 1;
    parameter TOTAL_NINSERTS = MCMI_EN ? mods.TOP.PLATFORM.COSIM.MCMI.TOTAL_NINSERTS : 1;
    parameter TOTAL_NWRITES = MCMI_EN ? mods.TOP.PLATFORM.COSIM.MCMI.TOTAL_NWRITES : 1;
    parameter int NREADS[NHARTS] = '{3}; //mods.TOP.PLATFORM.COSIM.MCMI.NREADS;
    parameter int NINSERTS[NHARTS] = '{3}; //mods.TOP.PLATFORM.COSIM.MCMI.NINSERTS;
    parameter int NWRITES[NHARTS] = '{3}; //mods.TOP.PLATFORM.COSIM.MCMI.NWRITES;

    typedef struct packed {
        logic                       valid;
        logic [15:0]                hart ;
        logic [64-1:0]              order;
        logic [PALEN-1:0]           addr ;
        logic [(CLLEN/8)-1:0]       mask ;
        logic [CLLEN-1:0]           data ;
    } mcmi_t;

    // --------------------------------------
    // PMCI - Performance Monitoring Counters
    // --------------------------------------
    parameter bit PMCI_EN = mods.TOP.PLATFORM.PMCI.ENABLE == 1;

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
        LOAD_MISAL_ACCESSES,
        STORE_MISAL_ACCESSES,
        STLF_HITS,
        VECTOR_BUSY_CYCLES,
        EVENT_COUNT
    } pmc_event_t;
    
    typedef struct packed {
        logic [HARTLEN-1:0]     hart;
        logic [3:0]             counter;
    } pmc_counter_t;

    typedef pmc_counter_t [EVENT_COUNT-1:0] pmci_t;

    // --------------------------------------
    // rv_tester ports
    // --------------------------------------
`define _RV_TESTER_PORTS(input,output)                                                              \
    input                                    clk,                                                   \
    input                                    reset,                                                 \
    input  rv_tester_params::bootstrap_t     bootstrap,                                             \
    input  rv_tester_pkg::interrupt_t        interrupt    [rv_tester_params::NHARTS-1:0],           \
    output                                   debug_mode   [rv_tester_params::NHARTS-1:0],           \
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
    output rv_tester_params::rvfi_t          rvfi         [rv_tester_params::TOTAL_NRETS-1:0],      \
    output rv_tester_params::mcmi_t          mcmi_read    [rv_tester_params::TOTAL_NREADS-1:0],     \
    output rv_tester_params::mcmi_t          mcmi_insert  [rv_tester_params::TOTAL_NINSERTS-1:0],   \
    output rv_tester_params::mcmi_t          mcmi_write   [rv_tester_params::TOTAL_NWRITES-1:0],    \
    output rv_tester_params::pmci_t          pmci         [rv_tester_params::NHARTS-1:0],           \
    output rv_tester_params::axi_req_t       axi_req      [rv_tester_params::AXI_TOTAL-1:0],        \
    input  rv_tester_params::axi_rsp_t       axi_rsp      [rv_tester_params::AXI_TOTAL-1:0],        \
    input  rv_tester_params::axi_req_mst_t   axi_req_mst  [rv_tester_params::AXI_MST_TOTAL-1:0],    \
    output rv_tester_params::axi_rsp_mst_t   axi_rsp_mst  [rv_tester_params::AXI_MST_TOTAL-1:0]


`define RV_TESTER_VARS(topology)                                                                    \
    logic                                    clk;                                                   \
    logic                                    reset;                                                 \
    rv_tester_params::bootstrap_t            bootstrap;                                             \
    rv_tester_pkg::interrupt_t               interrupt    [rv_tester_params::NHARTS-1:0];           \
    logic                                    debug_mode   [rv_tester_params::NHARTS-1:0];           \
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
    rv_tester_params::rvfi_t                 rvfi          [rv_tester_params::TOTAL_NRETS-1:0];     \
    rv_tester_params::mcmi_t                 mcmi_read     [rv_tester_params::TOTAL_NREADS-1:0];    \
    rv_tester_params::mcmi_t                 mcmi_insert   [rv_tester_params::TOTAL_NINSERTS-1:0];  \
    rv_tester_params::mcmi_t                 mcmi_write    [rv_tester_params::TOTAL_NWRITES-1:0];   \
    rv_tester_params::pmci_t                 pmci          [rv_tester_params::NHARTS-1:0];          \
    rv_tester_params::axi_req_t              axi_req       [rv_tester_params::AXI_TOTAL-1:0];       \
    rv_tester_params::axi_rsp_t              axi_rsp       [rv_tester_params::AXI_TOTAL-1:0];       \
    rv_tester_params::axi_req_mst_t          axi_req_mst   [rv_tester_params::AXI_MST_TOTAL-1:0];   \
    rv_tester_params::axi_rsp_mst_t          axi_rsp_mst   [rv_tester_params::AXI_MST_TOTAL-1:0];

`define RV_TESTER_PORTS `_RV_TESTER_PORTS(input,output)

endpackage
