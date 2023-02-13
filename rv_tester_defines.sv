`define RV_TESTER_PARAMETERS(CFG)                                              \
                                                                               \
    parameter int unsigned AXI_STRB_WIDTH = CFG.AXI_DATA_WIDTH / 8,            \
                                                                               \
    parameter type axi_addr_t   = logic [CFG.AXI_ADDR_WIDTH-1:0],              \
    parameter type axi_data_t   = logic [CFG.AXI_DATA_WIDTH-1:0],              \
    parameter type axi_id_t     = logic [CFG.AXI_ID_WIDTH  -1:0],              \
    parameter type axi_strb_t   = logic [    AXI_STRB_WIDTH-1:0],              \
                                                                               \
    parameter type axi_atop_t   = logic [5:0],                                 \
    parameter type axi_burst_t  = logic [1:0],                                 \
    parameter type axi_resp_t   = logic [1:0],                                 \
    parameter type axi_len_t    = logic [7:0],                                 \
    parameter type axi_size_t   = logic [2:0],                                 \
                                                                               \
    parameter type bootstrap_t = struct packed {                               \
        logic  [CFG.VLEN-1:0] boot_addr;                                       \
    },                                                                         \
                                                                               \
    parameter type rvfi_t   = struct packed {                                  \
        logic                       valid    ;                                 \
        logic [64-1:0]              order    ;                                 \
        logic [CFG.ILEN-1:0]        insn     ;                                 \
        logic                       trap     ;                                 \
        logic [CFG.XLEN-1:0]        cause    ;                                 \
        logic                       halt     ;                                 \
        logic                       intr     ;                                 \
        logic [2-1:0]               mode     ;                                 \
        logic [2-1:0]               ixl      ;                                 \
        logic [5-1:0]               rs1_addr ;                                 \
        logic [5-1:0]               rs2_addr ;                                 \
        logic [CFG.XLEN-1:0]        rs1_rdata;                                 \
        logic [CFG.XLEN-1:0]        rs2_rdata;                                 \
        logic [5-1:0]               rd_addr  ;                                 \
        logic [CFG.XLEN-1:0]        rd_wdata ;                                 \
                                                                               \
        logic [CFG.XLEN-1:0]        pc_rdata ;                                 \
        logic [CFG.XLEN-1:0]        pc_wdata ;                                 \
                                                                               \
        logic [CFG.XLEN-1:0]        mem_addr ;                                 \
        logic [CFG.XLEN-1:0]        mem_paddr;                                 \
        logic [(CFG.XLEN/8)-1:0]    mem_rmask;                                 \
        logic [(CFG.XLEN/8)-1:0]    mem_wmask;                                 \
        logic [CFG.XLEN-1:0]        mem_rdata;                                 \
        logic [CFG.XLEN-1:0]        mem_wdata;                                 \
    },                                                                         \
                                                                               \
    parameter type mcmi_t    = struct packed {                                 \
        logic                       valid;                                     \
        logic [64-1:0]              order;                                     \
        logic [CFG.PA_WIDTH-1:0]    addr ;                                     \
        logic [8-1:0]               size ;                                     \
        logic [CFG.XLEN-1:0]        data ;                                     \
        logic                       data_src;                                  \
    },                                                                         \
                                                                               \
    parameter type axi_req_t = struct packed {                                 \
        logic                   ar_valid ;                                     \
        axi_id_t                ar_id    ;                                     \
        axi_addr_t              ar_addr  ;                                     \
        axi_len_t               ar_len   ;                                     \
        axi_size_t              ar_size  ;                                     \
        axi_burst_t             ar_burst ;                                     \
        logic                   ar_lock  ;                                     \
                                                                               \
        logic                   aw_valid ;                                     \
        axi_id_t                aw_id    ;                                     \
        axi_addr_t              aw_addr  ;                                     \
        axi_len_t               aw_len   ;                                     \
        axi_size_t              aw_size  ;                                     \
        axi_burst_t             aw_burst ;                                     \
        logic                   aw_lock  ;                                     \
        axi_atop_t              aw_atop  ;                                     \
                                                                               \
        logic                   w_valid  ;                                     \
        axi_data_t              w_data   ;                                     \
        axi_strb_t              w_strb   ;                                     \
        logic                   w_last   ;                                     \
                                                                               \
        logic                   b_ready  ;                                     \
        logic                   r_ready  ;                                     \
    },                                                                         \
                                                                               \
    parameter type axi_rsp_t = struct packed {                                 \
        logic                   b_valid  ;                                     \
        axi_id_t                b_id     ;                                     \
        axi_resp_t              b_resp   ;                                     \
                                                                               \
        logic                   r_valid  ;                                     \
        axi_id_t                r_id     ;                                     \
        axi_data_t              r_data   ;                                     \
        axi_resp_t              r_resp   ;                                     \
        logic                   r_last   ;                                     \
                                                                               \
        logic                   aw_ready ;                                     \
        logic                   ar_ready ;                                     \
        logic                   w_ready  ;                                     \
    }

`define _RV_TESTER_PORTS(input,output)                                         \
    input                             clk      ,                               \
    input                             reset    ,                               \
    input  bootstrap_t                bootstrap,                               \
    input  rv_tester_pkg::interrupt_t interrupt,                               \
    output                            debug_mode,                              \
    input                             terminate,                               \
                                                                               \
    output rvfi_t      rvfi_instr   [CFG.NRET],                                \
    output mcmi_t      mcmi_store   [CFG.STQ_PORTS],                           \
    output axi_req_t   axi_req      [CFG.AXI_PORTS],                           \
    input  axi_rsp_t   axi_rsp      [CFG.AXI_PORTS]


`define RV_TESTER_VARS(CFG)                                                    \
    logic                      clk      ;                                      \
    logic                      reset    ;                                      \
    bootstrap_t                bootstrap;                                      \
    rv_tester_pkg::interrupt_t interrupt;                                      \
    logic                      debug_mode;                                     \
    logic                      terminate;                                      \
                                                                               \
    rvfi_t      rvfi_instr   [CFG.NRET];                                       \
    mcmi_t      mcmi_store   [CFG.STQ_PORTS];                                  \
    axi_req_t   axi_req      [CFG.AXI_PORTS];                                  \
    axi_rsp_t   axi_rsp      [CFG.AXI_PORTS];

`define RV_TESTER_PORTS `_RV_TESTER_PORTS(input,output)
