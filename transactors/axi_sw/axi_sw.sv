module axi_sw #(

    parameter int unsigned ADDR_WIDTH = 32'd0,
    parameter int unsigned DATA_WIDTH = 32'd0,
    parameter int unsigned STRB_WIDTH = DATA_WIDTH / 8,
    parameter int unsigned ID_WIDTH   = 32'd0,

    parameter int unsigned TOPO_ID    =    -1,
    parameter int NUM                 =    -1,
    parameter string tag = "notag",
    parameter int unsigned R_Q_MAX    = 32'd2,

    parameter type addr_t   = logic [ADDR_WIDTH-1:0],
    parameter type data_t   = logic [DATA_WIDTH-1:0],
    parameter type id_t     = logic [ID_WIDTH  -1:0],
    parameter type strb_t   = logic [STRB_WIDTH-1:0],

    parameter type burst_t  = logic [1:0],
    parameter type atop_t   = logic [5:0],
    parameter type resp_t   = logic [1:0],
    parameter type len_t    = logic [7:0],
    parameter type size_t   = logic [2:0]

)(

    input  clk,
    input  reset_n,
    input  sys_reset,

    input  logic             axi_mst_ar_valid,
    input  id_t              axi_mst_ar_id,
    input  addr_t            axi_mst_ar_addr,
    input  len_t             axi_mst_ar_len,
    input  size_t            axi_mst_ar_size,
    input  burst_t           axi_mst_ar_burst,
    input  logic             axi_mst_ar_lock,

    input  logic             axi_mst_aw_valid,
    input  id_t              axi_mst_aw_id,
    input  addr_t            axi_mst_aw_addr,
    input  len_t             axi_mst_aw_len,
    input  size_t            axi_mst_aw_size,
    input  burst_t           axi_mst_aw_burst,
    input  atop_t            axi_mst_aw_atop,
    input  logic             axi_mst_aw_lock,

    input  logic             axi_mst_w_valid,
    input  data_t            axi_mst_w_data,
    input  strb_t            axi_mst_w_strb,
    input  logic             axi_mst_w_last,

    input  logic             axi_mst_b_ready,
    input  logic             axi_mst_r_ready,

    output logic             axi_slv_b_valid,
    output id_t              axi_slv_b_id,
    output resp_t            axi_slv_b_resp,

    output logic             axi_slv_r_valid,
    output id_t              axi_slv_r_id,
    output data_t            axi_slv_r_data,
    output resp_t            axi_slv_r_resp,
    output logic             axi_slv_r_last,

    output logic             axi_slv_aw_ready,
    output logic             axi_slv_ar_ready,
    /* verilator lint_off UNOPTFLAT */
    output logic             axi_slv_w_ready,
    /* verilator lint_on UNOPTFLAT */
    `RV_TESTER_TRANSACTIONS_OUTPUT_AXI_SW

);

    localparam RESP_OKAY   = 2'b00;
    localparam RESP_EXOKAY = 2'b01;

    typedef struct packed {
        id_t   id  ;
        data_t data;
        resp_t resp;
        logic  last;
    } r_t;

    typedef byte unsigned dpi_data[DATA_WIDTH/$bits(byte)];
    typedef byte unsigned dpi_strb[STRB_WIDTH/$bits(byte)];

    localparam int unsigned R_FIFO_DEPTH = R_Q_MAX;

    typedef logic[$clog2(R_FIFO_DEPTH  )-1:0] r_queue_idx_t;
    typedef logic[$clog2(R_FIFO_DEPTH+1)-1:0] r_queue_ptr_t;
    localparam r_queue_ptr_t RFD = r_queue_ptr_t'(R_FIFO_DEPTH);

    typedef int     unsigned UI;
    typedef byte    unsigned UB;
    typedef longint unsigned UL;

    import "DPI-C" context function void axi_sw_set_scope(int unsigned location);

    r_t r_queue[R_FIFO_DEPTH];
    r_queue_ptr_t  r_queue_size, r_queue_wptr, r_queue_rptr, r_queue_wptr_nxt;

    logic b_queue_full     , b_queue_empty     ;
    logic w_last_queue_full, w_last_queue_empty;
    logic r_queue_full     , r_queue_empty     ;

    int unsigned location = cvm_topology::nil;
    always @(posedge clk) begin
        if (sys_reset) begin
            /* verilator lint_off BLKSEQ */
                // FIXME add a reset for the axi xtor
            location = cvm_topology::get_location(TOPO_ID, NUM);
            if (location != cvm_topology::nil) begin
                axi_sw_set_scope(location);
            end
            /* verilator lint_on BLKSEQ */
        end
    end

    function void axi_sw_r (int unsigned id, byte unsigned resp, dpi_data data, byte unsigned last);
        data_t d;
        r_t r;
        `ifndef IMMEDIATE_ASSERTIONS_IN_DPI_UNSUPPORTED
        assert(r_queue_wptr_nxt - r_queue_rptr != RFD);
        `endif
        // stream pack unsupported by verilator
        for (int i = 0; i < $size(dpi_data); i++) begin
            d[8*i +: 8] = data[i];
        end
        r = '{id: id_t'(id), data: data_t'(d), resp: 2'(resp), last: 1'(last)};
        r_queue[r_queue_idx_t'(r_queue_wptr_nxt % RFD)] = r;
        r_queue_wptr_nxt++;
    endfunction
    export "DPI-C" function axi_sw_r;

    assign r_queue_size  = r_queue_wptr - r_queue_rptr;
    assign r_queue_full  = r_queue_size == RFD;
    assign r_queue_empty = r_queue_size == 0;

    always_comb begin
        automatic r_t r = r_queue[r_queue_idx_t'(r_queue_rptr % RFD)];
        axi_slv_r_id    = r.id  ;
        axi_slv_r_data  = r.data;
        axi_slv_r_resp  = r.resp;
        axi_slv_r_last  = r.last;
    end

    logic r_queue_rptr_incremented;

    logic b_queue_aw_lock;

    assign axi_slv_aw_ready = !b_queue_full;
    assign axi_slv_ar_ready = '1;
    assign axi_slv_w_ready  = !axi_mst_w_last || !w_last_queue_full;
    assign axi_slv_b_valid  = !b_queue_empty && !w_last_queue_empty;
    assign axi_slv_b_resp   = b_queue_aw_lock ? RESP_EXOKAY : RESP_OKAY;
    assign axi_slv_r_valid  = !r_queue_empty;

    axi_sw_fifo #(
        .D         (1),
        .T         (logic[$bits(id_t)+1-1:0])
    ) b_queue (
        .clk         (clk                                 ),
        .reset_n     (reset_n                             ),
        .full        (b_queue_full                        ),
        .empty       (b_queue_empty                       ),
        .d           ({axi_mst_aw_id, axi_mst_aw_lock}    ),
        .push        (axi_mst_aw_valid && axi_slv_aw_ready),
        .q           ({axi_slv_b_id , b_queue_aw_lock}    ),
        .pop         (axi_slv_b_valid && axi_mst_b_ready  )
    );

    axi_sw_fifo #(
        .D         (1),
        .T         (logic)
    ) w_last_queue (
        .clk         (clk                                                 ),
        .reset_n     (reset_n                                             ),
        .full        (w_last_queue_full                                   ),
        .empty       (w_last_queue_empty                                  ),
        .d           (1'b1                                                ),
        .push        (axi_mst_w_valid && axi_slv_w_ready && axi_mst_w_last),
        .q           (                                                    ),
        .pop         (axi_slv_b_valid && axi_mst_b_ready                  )
    );

    always @(posedge clk) begin
        aws[0].valid      <= '0;
        ars[0].valid      <= '0;
        ws[0].valid       <= '0;
        r_q_ptrs[0].valid <= '0;

        if (reset_n) begin
            if (axi_slv_r_valid && axi_mst_r_ready) begin
                automatic r_queue_ptr_t n = r_queue_rptr + r_queue_ptr_t'(1);
                r_queue_rptr <= n;
                r_queue_rptr_incremented <= '1;
            end else begin
                r_queue_rptr_incremented <= '0;
            end
            if (r_queue_rptr_incremented) begin
                r_q_ptrs[0].valid         <= '1 & (location != cvm_topology::nil);
                r_q_ptrs[0].data.location <= location;
                r_q_ptrs[0].data.r_ptr    <= r_queue_rptr;
            end
        end else begin
            r_queue_rptr <= '0;
            /* verilator lint_off BLKANDNBLK */
            r_queue_wptr_nxt <= 0;
            /* verilator lint_on BLKANDNBLK */
        end
        r_queue_wptr <= r_queue_wptr_nxt;

        if (reset_n) begin
            if (axi_mst_aw_valid && axi_slv_aw_ready) begin
                aws[0].valid          <= '1 & (location != cvm_topology::nil);
                aws[0].data.location  <= location;
                aws[0].data.id        <= axi_mst_aw_id;
                aws[0].data.addr      <= axi_mst_aw_addr;
                aws[0].data.len       <= axi_mst_aw_len;
                aws[0].data.size      <= axi_mst_aw_size;
                aws[0].data.burst     <= axi_mst_aw_burst;
                aws[0].data.lock      <= axi_mst_aw_lock;
                aws[0].data.atop      <= axi_mst_aw_atop;
            end
            if (axi_mst_ar_valid && axi_slv_ar_ready) begin
                ars[0].valid          <= '1 & (location != cvm_topology::nil);
                ars[0].data.location  <= location;
                ars[0].data.id        <= axi_mst_ar_id;
                ars[0].data.addr      <= axi_mst_ar_addr;
                ars[0].data.len       <= axi_mst_ar_len;
                ars[0].data.size      <= axi_mst_ar_size;
                ars[0].data.burst     <= axi_mst_ar_burst;
                ars[0].data.lock      <= axi_mst_ar_lock;
            end
            if (axi_mst_w_valid && axi_slv_w_ready) begin
                ws[0].valid          <= '1 & (location != cvm_topology::nil);
                ws[0].data.location  <= location;
                ws[0].data.data      <= axi_mst_w_data;
                ws[0].data.strb      <= axi_mst_w_strb;
                ws[0].data.last      <= axi_mst_w_last;
            end
        end
    end

endmodule

module axi_sw_fifo #(
    parameter int unsigned D = 1    ,
    parameter type         T = logic
)(
    input  logic      clk,
    input  logic      reset_n,
    input  logic      push,
    input  T          d,
    input  logic      pop,
    output T          q,
    output logic      full,
    output logic      empty
);

    if ((D & (D-1)) != 0)
        $error("Depth %0d not a power of 2, modulo operator below is going to be more gates", D);

    typedef logic [$clog2(D+1)-1:0] ptr_t;

    ptr_t size, rptr, wptr;

    T ram[D];

    assign size  = wptr - rptr;
    assign full  = size == ptr_t'(D);
    assign empty = !size;

    always_ff @(posedge clk) begin
        if (reset_n) begin
            rptr <= rptr + ptr_t'(pop );
            wptr <= wptr + ptr_t'(push);
            if (push) begin
                ram[wptr % ptr_t'(D)] <= d;
            end
        end else begin
            rptr <= '0;
            wptr <= '0;
        end
    end

    push_when_full: assert property(@(posedge clk) disable iff(!reset_n) push -> !full)
        else $error("pushing when fifo is full");
    pop_when_empty: assert property(@(posedge clk) disable iff(!reset_n) pop  -> !empty)
        else $error("popping when fifo is empty");

    assign q = ram[rptr % ptr_t'(D)];

endmodule
