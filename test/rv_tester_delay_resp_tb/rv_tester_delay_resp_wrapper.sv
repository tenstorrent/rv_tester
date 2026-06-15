`include "axi/typedef.svh"

module rv_tester_delay_resp_wrapper #(
    parameter int unsigned AxiIdWidth = 8,
    parameter int unsigned AxiAddrWidth = 64, 
    parameter int unsigned AxiDataWidth = 64,
    parameter int unsigned AxiStrbWidth = 8,
    parameter int unsigned AxiUserWidth = 1,
    parameter int unsigned MaxInFlight = 16,
    parameter int unsigned MaxBeatsPerBurst = 16,
    parameter int CW = 32
)(
    input  logic clk,
    input  logic rst_ni,
    input  logic [CW-1:0] delay_cycles,
    input  logic [CW-1:0] delay_cycles_w,

    // AXI Slave Interface (AR channel only)
    input  logic [AxiIdWidth-1:0] slv_req_ar_id,
    input  logic [AxiAddrWidth-1:0] slv_req_ar_addr,
    input  logic [7:0] slv_req_ar_len,
    input  logic [2:0] slv_req_ar_size,
    input  logic [1:0] slv_req_ar_burst,
    input  logic slv_req_ar_lock,
    input  logic [3:0] slv_req_ar_cache,
    input  logic [2:0] slv_req_ar_prot,
    input  logic [3:0] slv_req_ar_qos,
    input  logic [3:0] slv_req_ar_region,
    input  logic [AxiUserWidth-1:0] slv_req_ar_user,
    input  logic slv_req_ar_valid,
    output logic slv_req_ar_ready,

    // AXI Slave Interface (AW channel)
    input  logic [AxiIdWidth-1:0] slv_req_aw_id,
    input  logic [AxiAddrWidth-1:0] slv_req_aw_addr,
    input  logic [7:0] slv_req_aw_len,
    input  logic [2:0] slv_req_aw_size,
    input  logic [1:0] slv_req_aw_burst,
    input  logic slv_req_aw_lock,
    input  logic [3:0] slv_req_aw_cache,
    input  logic [2:0] slv_req_aw_prot,
    input  logic [3:0] slv_req_aw_qos,
    input  logic [3:0] slv_req_aw_region,
    input  logic [AxiUserWidth-1:0] slv_req_aw_user,
    input  logic slv_req_aw_valid,
    output logic slv_req_aw_ready,

    // AXI Slave Response (R channel)
    output logic [AxiIdWidth-1:0] slv_resp_r_id,
    output logic [AxiDataWidth-1:0] slv_resp_r_data,
    output logic [1:0] slv_resp_r_resp,
    output logic slv_resp_r_last,
    output logic [AxiUserWidth-1:0] slv_resp_r_user,
    output logic slv_resp_r_valid,
    input  logic slv_req_r_ready,
    
    // AXI Slave Response (B channel) - for write responses
    output logic [AxiIdWidth-1:0] slv_resp_b_id,
    output logic [1:0] slv_resp_b_resp,
    output logic slv_resp_b_valid,
    input  logic slv_req_b_ready,
    
    // AXI Master Interface (AR channel)
    output logic [AxiIdWidth-1:0] mst_req_ar_id,
    output logic [AxiAddrWidth-1:0] mst_req_ar_addr,
    output logic [7:0] mst_req_ar_len,
    output logic [2:0] mst_req_ar_size,
    output logic [1:0] mst_req_ar_burst,
    output logic mst_req_ar_lock,
    output logic [3:0] mst_req_ar_cache,
    output logic [2:0] mst_req_ar_prot,
    output logic [3:0] mst_req_ar_qos,
    output logic [3:0] mst_req_ar_region,
    output logic [AxiUserWidth-1:0] mst_req_ar_user,
    output logic mst_req_ar_valid,
    input  logic mst_resp_ar_ready,

    // AXI Master Interface (AW channel)
    output logic [AxiIdWidth-1:0] mst_req_aw_id,
    output logic [AxiAddrWidth-1:0] mst_req_aw_addr,
    output logic [7:0] mst_req_aw_len,
    output logic [2:0] mst_req_aw_size,
    output logic [1:0] mst_req_aw_burst,
    output logic mst_req_aw_lock,
    output logic [3:0] mst_req_aw_cache,
    output logic [2:0] mst_req_aw_prot,
    output logic [3:0] mst_req_aw_qos,
    output logic [3:0] mst_req_aw_region,
    output logic [AxiUserWidth-1:0] mst_req_aw_user,
    output logic mst_req_aw_valid,
    input  logic mst_resp_aw_ready,

    // AXI Master Response (R channel)
    input  logic [AxiIdWidth-1:0] mst_resp_r_id,
    input  logic [AxiDataWidth-1:0] mst_resp_r_data,
    input  logic [1:0] mst_resp_r_resp,
    input  logic [AxiUserWidth-1:0] mst_resp_r_user,
    input  logic mst_resp_r_last,
    input  logic mst_resp_r_valid,
    output logic mst_req_r_ready,
    
    // AXI Master Response (B channel)
    input  logic [AxiIdWidth-1:0] mst_resp_b_id,
    input  logic [1:0] mst_resp_b_resp,
    input  logic mst_resp_b_user,
    input  logic mst_resp_b_valid,
    output logic mst_req_b_ready
);

    // AXI type definitions
    typedef logic [AxiIdWidth-1:0] id_slv_t;
    typedef logic [AxiIdWidth + $clog2(MaxInFlight)-1:0] id_mst_t;
    typedef logic [AxiAddrWidth-1:0] addr_t;
    typedef logic [AxiDataWidth-1:0] data_t;
    typedef logic [AxiStrbWidth-1:0] strb_t;
    typedef logic [AxiUserWidth-1:0] user_t;

    // AXI channel typedefs
    `AXI_TYPEDEF_AW_CHAN_T(slv_aw_chan_t, addr_t, id_slv_t, user_t)
    `AXI_TYPEDEF_AW_CHAN_T(mst_aw_chan_t, addr_t, id_slv_t, user_t)
    `AXI_TYPEDEF_W_CHAN_T(w_chan_t, data_t, strb_t, user_t)
    `AXI_TYPEDEF_B_CHAN_T(slv_b_chan_t, id_slv_t, user_t)
    `AXI_TYPEDEF_B_CHAN_T(mst_b_chan_t, id_mst_t, user_t)  // B channel uses wider ID for master
    `AXI_TYPEDEF_AR_CHAN_T(slv_ar_chan_t, addr_t, id_slv_t, user_t)
    `AXI_TYPEDEF_AR_CHAN_T(mst_ar_chan_t, addr_t, id_mst_t, user_t)
    `AXI_TYPEDEF_R_CHAN_T(slv_r_chan_t, data_t, id_slv_t, user_t)
    `AXI_TYPEDEF_R_CHAN_T(mst_r_chan_t, data_t, id_mst_t, user_t)
    `AXI_TYPEDEF_REQ_T(slv_req_t, slv_aw_chan_t, w_chan_t, slv_ar_chan_t)
    `AXI_TYPEDEF_REQ_T(mst_req_t, mst_aw_chan_t, w_chan_t, mst_ar_chan_t)
    `AXI_TYPEDEF_RESP_T(slv_resp_t, slv_b_chan_t, slv_r_chan_t)
    `AXI_TYPEDEF_RESP_T(mst_resp_t, mst_b_chan_t, mst_r_chan_t)

    // Internal signals
    slv_ar_chan_t slv_req_ar;
    slv_aw_chan_t slv_req_aw;
    slv_resp_t slv_resp;
    mst_resp_t mst_resp;

    // Connect AR channel
    assign slv_req_ar.id = slv_req_ar_id;
    assign slv_req_ar.addr = slv_req_ar_addr;
    assign slv_req_ar.len = slv_req_ar_len;
    assign slv_req_ar.size = slv_req_ar_size;
    assign slv_req_ar.burst = slv_req_ar_burst;
    assign slv_req_ar.lock = slv_req_ar_lock;
    assign slv_req_ar.cache = slv_req_ar_cache;
    assign slv_req_ar.prot = slv_req_ar_prot;
    assign slv_req_ar.qos = slv_req_ar_qos;
    assign slv_req_ar.region = slv_req_ar_region;
    assign slv_req_ar.user = slv_req_ar_user;

    // Connect AW channel
    assign slv_req_aw.id = slv_req_aw_id;
    assign slv_req_aw.addr = slv_req_aw_addr;
    assign slv_req_aw.len = slv_req_aw_len;
    assign slv_req_aw.size = slv_req_aw_size;
    assign slv_req_aw.burst = slv_req_aw_burst;
    assign slv_req_aw.lock = slv_req_aw_lock;
    assign slv_req_aw.cache = slv_req_aw_cache;
    assign slv_req_aw.prot = slv_req_aw_prot;
    assign slv_req_aw.qos = slv_req_aw_qos;
    assign slv_req_aw.region = slv_req_aw_region;
    assign slv_req_aw.user = slv_req_aw_user;

    // Connect R channel response
    assign slv_resp_r_id = slv_resp.r.id;
    assign slv_resp_r_data = slv_resp.r.data;
    assign slv_resp_r_resp = slv_resp.r.resp;
    assign slv_resp_r_last = slv_resp.r.last;
    assign slv_resp_r_user = slv_resp.r.user;
    assign slv_resp_r_valid = slv_resp.r_valid;

    // Connect B channel response
    assign slv_resp_b_id = slv_resp.b.id;
    assign slv_resp_b_resp = slv_resp.b.resp;
    assign slv_resp_b_valid = slv_resp.b_valid;

    // Connect master response
    assign mst_resp.r.id = id_mst_t'(mst_resp_r_id);
    assign mst_resp.r.data = mst_resp_r_data;
    assign mst_resp.r.resp = mst_resp_r_resp;
    assign mst_resp.r.last = mst_resp_r_last;
    assign mst_resp.r.user = mst_resp_r_user;
    assign mst_resp.r_valid = mst_resp_r_valid;
    assign mst_req_r_ready = 1'b1;  // Always ready to accept responses

    assign mst_resp.b.id = id_mst_t'(mst_resp_b_id);
    assign mst_resp.b.resp = mst_resp_b_resp;
    assign mst_resp.b.user = mst_resp_b_user;
    assign mst_resp.b_valid = mst_resp_b_valid;
    assign mst_req_b_ready = 1'b1;  // Always ready to accept responses
    assign mst_resp.ar_ready = mst_resp_ar_ready;
    assign mst_req_ar_valid = slv_req_ar_valid;

    // Connect master AR outputs (need to truncate the wider ID)
    assign mst_req_ar_addr = slv_req_ar_addr;
    assign mst_req_ar_len = slv_req_ar_len;
    assign mst_req_ar_size = slv_req_ar_size;
    assign mst_req_ar_burst = slv_req_ar_burst;
    assign mst_req_ar_lock = slv_req_ar_lock;
    assign mst_req_ar_cache = slv_req_ar_cache;
    assign mst_req_ar_prot = slv_req_ar_prot;
    assign mst_req_ar_qos = slv_req_ar_qos;
    assign mst_req_ar_region = slv_req_ar_region;
    assign mst_req_ar_user = slv_req_ar_user;

    assign mst_resp.aw_ready = mst_resp_aw_ready;
    assign mst_req_aw_valid = slv_req_aw_valid;

    // Connect master AW outputs
    assign mst_req_aw_addr = slv_req_aw_addr;
    assign mst_req_aw_len = slv_req_aw_len;
    assign mst_req_aw_size = slv_req_aw_size;
    assign mst_req_aw_burst = slv_req_aw_burst;
    assign mst_req_aw_lock = slv_req_aw_lock;
    assign mst_req_aw_cache = slv_req_aw_cache;
    assign mst_req_aw_prot = slv_req_aw_prot;
    assign mst_req_aw_qos = slv_req_aw_qos;
    assign mst_req_aw_region = slv_req_aw_region;
    assign mst_req_aw_user = slv_req_aw_user;

    // DUT instantiation
    rv_tester_delay_resp #(
        .AxiIdWidth(AxiIdWidth),
        .slv_resp_t(slv_resp_t),
        .mst_resp_t(mst_resp_t),
        .r_chan_t(mst_r_chan_t),
        .b_chan_t(mst_b_chan_t),
        .slv_ar_chan_t(slv_ar_chan_t),
        .slv_aw_chan_t(slv_aw_chan_t),
        .MaxInFlight(MaxInFlight),
        .MaxBeatsPerBurst(MaxBeatsPerBurst),
        .CW(CW)
    ) dut (
        .clk_i(clk),
        .rst_ni(rst_ni),
        .delay_cycles(delay_cycles),
        .delay_cycles_w(delay_cycles_w),
        .slv_req_ar_i(slv_req_ar),
        .slv_req_ar_valid_i(slv_req_ar_valid),
        .slv_req_r_ready_i(slv_req_r_ready),
        .slv_req_aw_i(slv_req_aw),
        .slv_req_aw_valid_i(slv_req_aw_valid),
        .slv_req_b_ready_i(slv_req_b_ready),
        .slv_resp_o(slv_resp),
        .mst_req_ar_id_o(mst_req_ar_id),
        .mst_req_aw_id_o(mst_req_aw_id),
        .mst_resp_i(mst_resp)
    );

    // Connect ready signals
    assign slv_req_ar_ready = slv_resp.ar_ready;
    assign slv_req_aw_ready = slv_resp.aw_ready;

endmodule 
