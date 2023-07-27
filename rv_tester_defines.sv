package rv_tester_params;

    import topology_pkg::mods;

    typedef logic [mods.TOP.PLATFORM.AXI.ADDR_WIDTH-1:0] axi_addr_t;
    typedef logic [mods.TOP.PLATFORM.AXI.DATA_WIDTH-1:0] axi_data_t;
    typedef logic [mods.TOP.PLATFORM.AXI.STRB_WIDTH-1:0] axi_strb_t;
    typedef logic [mods.TOP.PLATFORM.AXI.ID_WIDTH  -1:0] axi_id_t;

    typedef logic [mods.TOP.PLATFORM.AXI_MST.ADDR_WIDTH-1:0] axi_mst_addr_t;
    typedef logic [mods.TOP.PLATFORM.AXI_MST.DATA_WIDTH-1:0] axi_mst_data_t;
    typedef logic [mods.TOP.PLATFORM.AXI_MST.STRB_WIDTH-1:0] axi_mst_strb_t;
    typedef logic [mods.TOP.PLATFORM.AXI_MST.ID_WIDTH  -1:0] axi_mst_id_t;

    typedef logic [5:0] axi_atop_t;
    typedef logic [1:0] axi_burst_t;
    typedef logic [1:0] axi_resp_t;
    typedef logic [7:0] axi_len_t;
    typedef logic [2:0] axi_size_t;

    typedef struct packed {
        logic  [mods.TOP.CLUSTER.CORE.VLEN-1:0] boot_addr;
    } bootstrap_t;

    typedef struct packed {
        logic                                             valid    ;
        logic                                             last_uop ;
        logic                                             comp     ;
        logic [64-1:0]                                    order    ;
        logic [mods.TOP.CLUSTER.CORE.ILEN-1:0]            insn     ;
        logic [64-1:0]                                    uop      ;
        logic                                             trap     ;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            cause    ;
        logic                                             halt     ;
        logic                                             intr     ;
        logic [2-1:0]                                     mode     ;
        logic [2-1:0]                                     ixl      ;
        logic [5-1:0]                                     rs1_addr ;
        logic [5-1:0]                                     rs2_addr ;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            rs1_rdata;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            rs2_rdata;
        logic [5-1:0]                                     rd_addr  ;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            rd_wdata ;
        logic                                             frd_valid;
        logic [5-1:0]                                     frd_addr ;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            frd_wdata;

        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            pc_rdata ;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            pc_wdata ;

        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            mem_addr ;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            mem_paddr;
        logic [(mods.TOP.CLUSTER.CORE.XLEN/8)-1:0]        mem_rmask;
        logic [(mods.TOP.CLUSTER.CORE.XLEN/8)-1:0]        mem_wmask;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            mem_rdata;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]            mem_wdata;
    } rvfi_t;

    typedef struct packed {
        logic                                             valid;
        logic [64-1:0]                                    order;
        logic [mods.TOP.CLUSTER.CORE.PA_WIDTH-1:0]    addr ;
        logic [8-1:0]                                     size ;
        logic [mods.TOP.CLUSTER.CORE.XLEN-1:0]        data ;
        logic                                          data_src;
    } mcmi_t;

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

    typedef logic [3:0]   pmu_event_t;
    typedef enum {
        L1D_READ_ACCESS,
        L1D_WRITE_ACCESS
    } pmu_event_id_t;

`define _RV_TESTER_PORTS(input,output)                                                                       \
    input                                               clk      ,                                           \
    input                                               reset    ,                                           \
    input  rv_tester_params::bootstrap_t                bootstrap,                                           \
    input  rv_tester_pkg::interrupt_t                   interrupt,                                           \
    input  rv_tester_pkg::dm_write_t                    dmi_write,                                           \
    output                                              debug_mode,                                          \
    input                                               terminate,                                           \
    input  logic                                        terminated,                                          \
    output                                              quiesced,                                            \
                                                                                                             \
    output rv_tester_params::rvfi_t          rvfi_instr   [topology.TOP.CLUSTER.CORE.NRET],                  \
    output rv_tester_params::mcmi_t          mcmi_store   [topology.TOP.CLUSTER.CORE.STQ_PORTS],             \
    output rv_tester_params::pmu_event_t     pmu_event    [topology.TOP.PLATFORM.PMU.EVENT_COUNT],           \
    output rv_tester_params::axi_req_t       axi_req      [topology.TOP.PLATFORM.AXI.TOTAL],                 \
    input  rv_tester_params::axi_rsp_t       axi_rsp      [topology.TOP.PLATFORM.AXI.TOTAL],                 \
    input  rv_tester_params::axi_req_mst_t   axi_req_mst  [topology.TOP.PLATFORM.AXI_MST.TOTAL],             \
    output rv_tester_params::axi_rsp_mst_t   axi_rsp_mst  [topology.TOP.PLATFORM.AXI_MST.TOTAL]


`define RV_TESTER_VARS(topology)                                                                             \
    logic                                        clk      ;                                                  \
    logic                                        reset    ;                                                  \
    rv_tester_params::bootstrap_t                bootstrap;                                                  \
    rv_tester_pkg::interrupt_t                   interrupt;                                                  \
    rv_tester_pkg::dm_write_t                    dmi_write;                                                  \
    logic                                        debug_mode;                                                 \
    logic                                        terminate;                                                  \
    logic                                        terminated;                                                 \
    logic                                        quiesced;                                                   \
                                                                                                             \
    rv_tester_params::rvfi_t          rvfi_instr   [topology.TOP.CLUSTER.CORE.NRET];                         \
    rv_tester_params::mcmi_t          mcmi_store   [topology.TOP.CLUSTER.CORE.STQ_PORTS];                    \
    rv_tester_params::pmu_event_t     pmu_event    [topology.TOP.PLATFORM.PMU.EVENT_COUNT];                  \
    rv_tester_params::axi_req_t       axi_req      [topology.TOP.PLATFORM.AXI.TOTAL];                        \
    rv_tester_params::axi_rsp_t       axi_rsp      [topology.TOP.PLATFORM.AXI.TOTAL];                        \
    rv_tester_params::axi_req_mst_t   axi_req_mst  [topology.TOP.PLATFORM.AXI_MST.TOTAL];                    \
    rv_tester_params::axi_rsp_mst_t   axi_rsp_mst  [topology.TOP.PLATFORM.AXI_MST.TOTAL];

`define RV_TESTER_PORTS `_RV_TESTER_PORTS(input,output)

endpackage
