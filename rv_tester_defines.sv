///////////////includes///////////////////////////////

`include "axi/typedef.svh"
/////////////////////////////////////////////////////

package rv_tester_params;

    import cvm_topology_gen::mods;

    `RV_TESTER_AXI_ENUMS
    `RV_TESTER_AXI_TYPEDEFS
    // AXI_TOTAL = the AXI (mem) group's per-parent instance count, i.e. the
    // size of axi_req[]. Use SHARD (group count), not TOTAL (sum across
    // all groups, which would also include NCIO etc).
    parameter AXI_TOTAL = mods.TOP.PLATFORM.AXI_SW[AXI_IDX].SHARD;

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
        logic                       mem_page4kX;
        logic [32-1:0]              mem_page4kX_attr ;
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
        logic  [7:0]                user ;
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
    typedef enum {C2,INTERRUPT,TRIGGER_COUNT} event_trigger_type_t;
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
    // Pwrmgmt
    // --------------------------------------
    parameter bit PWRMGMT_EN = mods.TOP.PLATFORM.PWRMGMT.PWRMGMT_EN == 1;

    // --------------------------------------
    // typedefs to generate all data types
    // --------------------------------------

    parameter AXI_USER_ID_WIDTH = 1;
    typedef logic [AXI_USER_ID_WIDTH-1:0] user_t;

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
    output  logic                            warm_reset_req,                                        \
    input   logic                            core_terminate_conditions,                             \
    output  logic                            cold_reset,                                            \
    output  logic                            warm_reset,                                            \
    output  int                              num_resets,                                            \
    output  logic                            warm_reset_en,                                         \
    output  int                              target_num_resets,                                     \
    output logic                             reset_window,                                          \
    output                                   warm_reset_release_hang,                               \
    output                                   force_ref_clk,                                         \
    output [rv_tester_params::NHARTS-1:0]    core_no_fetch,                                         \
    input  [rv_tester_params::NRESETS-1:0]   reset, /*Packed so zebu can easily force*/             \
    input  rv_tester_params::bootstrap_t     bootstrap,                                             \
    input  rv_tester_pkg::nmi_t              nmi                [rv_tester_params::NHARTS-1:0],     \
    output rv_tester_pkg::nmi_t              nmi_pend           [rv_tester_params::NHARTS-1:0],     \
    input  rv_tester_pkg::interrupt_t        interrupt          [rv_tester_params::NHARTS-1:0],     \
    output rv_tester_params::interrupt_pend_t interrupt_pend    [rv_tester_params::NHARTS-1:0],     \
    output rv_tester_pkg::mtimeMmr_t         mtime,                                                 \
    output logic [63:0]                      timeCsr            [rv_tester_params::NHARTS-1:0],     \
    output logic                             MTIP               [rv_tester_params::NHARTS-1:0],     \
    output rv_tester_params::msi_t           imsic_msi          [rv_tester_params::NHARTS-1:0],     \
    output                                   debug_mode         [rv_tester_params::NHARTS-1:0],     \
    output                                   disable_checks,                                        \
    output                                   dut_terminate,                                         \
    input                                    cvm_done,                                              \
    `ifdef UVM_MACROS_SVH                                                                           \
    output                                   uvm_done,                                              \
    `endif                                                                                          \
    input                                    terminate,                                             \
    input                                    sysmod_terminate,                                      \
    input  logic                             terminated,                                            \
    input  logic                             terminate_now,                                         \
    output logic                             ntrace_terminate,                                      \
    output logic                             terminate_dst_trace_seq,                               \
    output logic                             dmi_terminate,                                         \
    output logic                             dmi_poll_timeout_terminate,                            \
    input  rv_tester_pkg::dm_write_t         dmi_write,                                             \
    output                                   quiesced,                                              \
    output                                   unconditional_terminate,                               \
    input                                    boot_done_all,                                         \
    input logic [64-1:0]                     cosim_eot_addr,                                        \
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
    output pmu_core_pkg::pmci_t              pmci         [rv_tester_params::NHARTS-1:0],           \
    output pmu_core_pkg::hpmi_t              hpmi         [rv_tester_params::NHARTS-1:0],           \
    output rv_tester_pkg::sc_pmci_t          sc_pmci,                                               \
    input  logic                             rv_tester_reset_,                                      \
    `RV_TESTER_AXI_PORTS(input, output, rv_tester_params),                                          \
    input  logic warm_reset_now,                                                   \
    input  logic sys_reset [rv_tester_params::NCLKS-1:0],                          \
    output rv_tester_params::event_trigger_intf_t event_triggers  [rv_tester_params::NHARTS-1:0], \
    input logic [rv_tester_params::NHARTS-1:0][31:0] cycles_since_retire


`define RV_TESTER_VARS(topology)                                                                    \
    logic [63:0]                             clocks;                                                \
    logic                                    clk             [rv_tester_params::NCLKS-1:0];         \
    logic                                    dut_clk         [rv_tester_params::NCLKS-1:0];         \
    logic                                    dut_reset       [rv_tester_params::NCLKS-1:0];         \
    logic                                    dut_reset_req;                                         \
    logic                                    ndmreset_ack;                                          \
    logic                                    dut_reset_req_active;                                  \
    logic                                    warm_reset_req;                                        \
    logic                                    core_terminate_conditions;                             \
    logic                                    cold_reset;                                            \
    logic                                    warm_reset;                                            \
    logic                                    warm_reset_en;                                         \
    int                                      num_resets;                                            \
    int                                      target_num_resets;                                     \
    logic                                    reset_window;                                          \
    logic                                    warm_reset_release_hang;                               \
    logic                                    force_ref_clk;                                         \
    logic [rv_tester_params::NHARTS-1:0]     core_no_fetch;                                         \
    logic [rv_tester_params::NRESETS-1:0]    reset           /* Packed so zebu can force easily */; \
    rv_tester_params::bootstrap_t            bootstrap;                                             \
    rv_tester_pkg::nmi_t                     nmi             [rv_tester_params::NHARTS-1:0];        \
    rv_tester_pkg::nmi_t                     nmi_pend        [rv_tester_params::NHARTS-1:0];        \
    rv_tester_pkg::interrupt_t               interrupt       [rv_tester_params::NHARTS-1:0];        \
    rv_tester_params::interrupt_pend_t       interrupt_pend  [rv_tester_params::NHARTS-1:0];        \
    rv_tester_pkg::mtimeMmr_t                mtime;                                                 \
    logic [63:0]                             timeCsr         [rv_tester_params::NHARTS-1:0];        \
    logic                                    MTIP            [rv_tester_params::NHARTS-1:0];        \
    rv_tester_params::msi_t                  imsic_msi       [rv_tester_params::NHARTS-1:0];        \
    logic                                    debug_mode      [rv_tester_params::NHARTS-1:0];        \
    logic                                    disable_checks;                                        \
    logic                                    dut_terminate;                                         \
    logic                                    terminate;                                             \
    logic                                    cvm_done;                                              \
    logic                                    sysmod_terminate;                                      \
    logic                                    terminated;                                            \
    logic                                    terminate_now;                                         \
    logic                                    terminate_dst_trace_seq;                               \
    logic                                    ntrace_terminate;                                      \
    logic                                    dmi_terminate;                                         \
    logic                                    dmi_poll_timeout_terminate;                            \
    logic                                    quiesced;                                              \
    logic                                    unconditional_terminate;                               \
    logic                                    boot_done_all;                                         \
    logic [64-1:0]                           cosim_eot_addr;                                        \
    logic [7:0]                              DM_DebugReq_Valids;                                    \
    logic                                    rv_tester_reset_;                                      \
    logic [51:0]                             dfetch_cl_addr[1:0];                                   \
    logic [1:0]                              dfetch_cl_valid;                                       \
    logic [51:0]                             writeback_cl_addr[1:0];                                \
    logic [1:0]                              writeback_cl_valid;                                    \
    logic [51:0]                             devict_cl_addr [rv_tester_params::NHARTS-1:0];         \
    logic                                    devict_cl_valid [rv_tester_params::NHARTS-1:0];        \
    logic [51:0]                             flush_cl_addr  [rv_tester_params::NHARTS-1:0];         \
    logic                                    flush_cl_valid [rv_tester_params::NHARTS-1:0];         \
    rv_tester_params::rvfi_t                 [rv_tester_params::TOTAL_NRETS-1:0]       rvfi;        \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NREADS-1:0]      mcmi_read;   \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NINSERTS-1:0]    mcmi_insert; \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NWRITES-1:0]     mcmi_write;  \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NBYPASSES-1:0]   mcmi_bypass; \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NIFETCHES-1:0]   mcmi_ifetch_req; \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NIFETCHES-1:0]   mcmi_ifetch_resp; \
    rv_tester_params::mcmi_t                 [rv_tester_params::TOTAL_NIEVICTS-1:0]    mcmi_ievict; \
    rv_tester_params::csri_t                 csri          [rv_tester_params::NHARTS-1:0];          \
    pmu_core_pkg::pmci_t                     pmci          [rv_tester_params::NHARTS-1:0];          \
    pmu_core_pkg::hpmi_t                     hpmi          [rv_tester_params::NHARTS-1:0];          \
    rv_tester_pkg::sc_pmci_t                 sc_pmci;                                               \
    `RV_TESTER_AXI_VARS(rv_tester_params)                                                           \
    logic warm_reset_now;                                                                           \
    logic sys_reset [rv_tester_params::NCLKS-1:0];                                                  \
    rv_tester_params::event_trigger_intf_t event_triggers [rv_tester_params::NHARTS-1:0];           \
    rv_tester_pkg::dm_write_t dmi_write;                                                            \
    logic [rv_tester_params::NHARTS-1:0][31:0] cycles_since_retire;

    

`define RV_TESTER_PORTS `_RV_TESTER_PORTS(input,output)


`define RV_TESTER_KEEPER_INIT(clk,num) \
   bit [num-1:0] rv_tester_keeper_data_vec;\
   bit           rv_tester_keeper_load, rv_tester_keeper_send, rv_tester_keeper_data;\
   assign rv_tester_keeper_data = | rv_tester_keeper_data_vec;\
   dpi_rv_tester_keeper_ctrl i_dpi_rv_tester_keeper_ctrl(clk , rv_tester_keeper_load, rv_tester_keeper_data);

`define RV_TESTER_KEEPER_DATA(clk,num,data) \
   bit [$bits(data)-1:0] data``_rv_tester_keeper_reg;\
   always @(posedge clk) data``_rv_tester_keeper_reg <= (rv_tester_keeper_load==1'b1) ? data : {1'b0,data``_rv_tester_keeper_reg[$bits(data)-1:1]};\
   assign rv_tester_keeper_data_vec[num-1] = data``_rv_tester_keeper_reg[0];



endpackage
