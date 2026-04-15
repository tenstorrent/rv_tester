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

    parameter IOMMU_AXI_TR_REQ_MST_TOTAL = mods.TOP.PLATFORM.IOMMU_AXI_TR_REQ_MST.TOTAL;
    parameter IOMMU_AXI_TR_REQ_MST_ADDR_WIDTH = mods.TOP.PLATFORM.IOMMU_AXI_TR_REQ_MST.ADDR_WIDTH;
    parameter IOMMU_AXI_TR_REQ_MST_DATA_WIDTH = mods.TOP.PLATFORM.IOMMU_AXI_TR_REQ_MST.DATA_WIDTH;
    parameter IOMMU_AXI_TR_REQ_MST_STRB_WIDTH = mods.TOP.PLATFORM.IOMMU_AXI_TR_REQ_MST.STRB_WIDTH;
    parameter IOMMU_AXI_TR_REQ_MST_ID_WIDTH = mods.TOP.PLATFORM.IOMMU_AXI_TR_REQ_MST.ID_WIDTH;
    parameter IOMMU_AXI_TR_REQ_MST_USER_WIDTH = mods.TOP.PLATFORM.IOMMU_AXI_TR_REQ_MST.USER_WIDTH;

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

    typedef logic [IOMMU_AXI_TR_REQ_MST_ADDR_WIDTH-1:0] iommu_axi_tr_req_addr_t;
    typedef logic [IOMMU_AXI_TR_REQ_MST_DATA_WIDTH-1:0] iommu_axi_tr_req_data_t;
    typedef logic [IOMMU_AXI_TR_REQ_MST_STRB_WIDTH-1:0] iommu_axi_tr_req_strb_t;
    typedef logic [IOMMU_AXI_TR_REQ_MST_ID_WIDTH  -1:0] iommu_axi_tr_req_id_t;
    typedef logic [IOMMU_AXI_TR_REQ_MST_USER_WIDTH-1:0] iommu_axi_tr_req_user_t;

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

    typedef struct packed {
        logic                       ar_valid ;
        iommu_axi_tr_req_id_t       ar_id    ;
        iommu_axi_tr_req_addr_t     ar_addr  ;
        axi_len_t                   ar_len   ;
        axi_size_t                  ar_size  ;
        axi_burst_t                 ar_burst ;
        logic                       ar_lock  ;
        
        logic                       aw_valid ;
        iommu_axi_tr_req_id_t       aw_id    ;
        iommu_axi_tr_req_addr_t     aw_addr  ;
        axi_len_t                   aw_len   ;
        axi_size_t                  aw_size  ;
        axi_burst_t                 aw_burst ;
        logic                       aw_lock  ;

        logic                       w_valid  ;
        iommu_axi_tr_req_data_t     w_data   ;
        iommu_axi_tr_req_strb_t     w_strb   ;
        logic                       w_last   ;

        logic                       b_ready  ;
        logic                       r_ready  ;
    } iommu_axi_tr_req_req_t;

    typedef struct packed {
        logic                       b_valid  ;
        iommu_axi_tr_req_id_t       b_id     ;
        axi_resp_t                  b_resp   ;

        logic                       r_valid  ;
        iommu_axi_tr_req_id_t       r_id     ;
        iommu_axi_tr_req_data_t     r_data   ;
        axi_resp_t                  r_resp   ;
        logic                       r_last   ;

        logic                       aw_ready ;
        logic                       ar_ready ;
        logic                       w_ready  ;
    } iommu_axi_tr_req_rsp_t;


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

    `AXI_TYPEDEF_AW_CHAN_T(iommu_axi_tr_req_aw_chan_top, iommu_axi_tr_req_addr_t, iommu_axi_tr_req_id_t, iommu_axi_tr_req_user_t)
    `AXI_TYPEDEF_W_CHAN_T(iommu_axi_tr_req_w_chan_top, iommu_axi_tr_req_data_t, iommu_axi_tr_req_strb_t, iommu_axi_tr_req_user_t)
    `AXI_TYPEDEF_B_CHAN_T(iommu_axi_tr_req_b_chan_top, iommu_axi_tr_req_id_t, iommu_axi_tr_req_user_t)
    `AXI_TYPEDEF_AR_CHAN_T(iommu_axi_tr_req_ar_chan_top, iommu_axi_tr_req_addr_t, iommu_axi_tr_req_id_t, iommu_axi_tr_req_user_t)
    `AXI_TYPEDEF_R_CHAN_T(iommu_axi_tr_req_r_chan_top, iommu_axi_tr_req_data_t, iommu_axi_tr_req_id_t, iommu_axi_tr_req_user_t)
    `AXI_TYPEDEF_REQ_T(iommu_axi_tr_req_req_top, iommu_axi_tr_req_aw_chan_top, iommu_axi_tr_req_w_chan_top, iommu_axi_tr_req_ar_chan_top)
    `AXI_TYPEDEF_RESP_T(iommu_axi_tr_req_rsp_top, iommu_axi_tr_req_b_chan_top, iommu_axi_tr_req_r_chan_top)

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
    input  logic                             rv_tester_reset_,                                       \
    output rv_tester_params::slv_req_top     axi_req [rv_tester_params::AXI_TOTAL-1:0],             \
    input  rv_tester_params::slv_resp_top    axi_rsp [rv_tester_params::AXI_TOTAL-1:0],             \
    output rv_tester_params::ncio_slv_req_top     ncio_axi_req [rv_tester_params::NCIO_AXI_TOTAL-1:0],             \
    input  rv_tester_params::ncio_slv_resp_top    ncio_axi_rsp [rv_tester_params::NCIO_AXI_TOTAL-1:0],             \
    input  rv_tester_params::mst_req_top     axi_req_mst [rv_tester_params::AXI_MST_TOTAL-1:0],     \
    output rv_tester_params::mst_resp_top    axi_rsp_mst [rv_tester_params::AXI_MST_TOTAL-1:0],     \
    input  rv_tester_params::smc_req_top    smc_axi_req_mst [rv_tester_params::SMC_AXI_MST_TOTAL-1:0],   \
    output rv_tester_params::smc_resp_top   smc_axi_rsp_mst [rv_tester_params::SMC_AXI_MST_TOTAL-1:0],   \
    input  rv_tester_params::iommu_axi_tr_req_req_top iommu_axi_tr_req_req_mst [rv_tester_params::IOMMU_AXI_TR_REQ_MST_TOTAL-1:0], \
    output rv_tester_params::iommu_axi_tr_req_rsp_top iommu_axi_tr_req_rsp_mst [rv_tester_params::IOMMU_AXI_TR_REQ_MST_TOTAL-1:0], \
    input  logic warm_reset_now,                                                   \
    input  logic sys_reset [rv_tester_params::NCLKS-1:0],                          \
    output rv_tester_params::event_trigger_intf_t event_triggers  [rv_tester_params::NHARTS-1:0], \
    input logic [rv_tester_params::NHARTS-1:0][31:0] cycles_since_retire

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
    rv_tester_params::iommu_axi_tr_req_req_top iommu_axi_tr_req_req_mst [rv_tester_params::IOMMU_AXI_TR_REQ_MST_TOTAL-1:0]; \
    rv_tester_params::iommu_axi_tr_req_rsp_top iommu_axi_tr_req_rsp_mst [rv_tester_params::IOMMU_AXI_TR_REQ_MST_TOTAL-1:0]; \
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
