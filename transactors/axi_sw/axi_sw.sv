module axi_sw #(

    parameter int unsigned ADDR_WIDTH = 32'd0,
    parameter int unsigned DATA_WIDTH = 32'd0,
    parameter int unsigned ID_WIDTH   = 32'd0,

    parameter bit POLL_DEFAULT        = 1'd0,
    parameter int SYSMOD_NUM          =   -1,
    parameter string tag = "notag",
    parameter int unsigned R_FIFO_DEPTH = 32'd2,

    parameter int unsigned STRB_WIDTH = DATA_WIDTH / 8,

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

    input  logic             axi_mst_ar_valid,
    input  id_t              axi_mst_ar_id,
    input  addr_t            axi_mst_ar_addr,
    input  len_t             axi_mst_ar_len,
    input  size_t            axi_mst_ar_size,
    input  burst_t           axi_mst_ar_burst,
    // verilator lint_off UNUSED
    input  logic             axi_mst_ar_lock,
    // verilator lint_on UNUSED

    input  logic             axi_mst_aw_valid,
    input  id_t              axi_mst_aw_id,
    input  addr_t            axi_mst_aw_addr,
    input  len_t             axi_mst_aw_len,
    input  size_t            axi_mst_aw_size,
    input  burst_t           axi_mst_aw_burst,
    // verilator lint_off UNUSED
    input  atop_t            axi_mst_aw_atop,
    input  logic             axi_mst_aw_lock,
    // verilator lint_on UNUSED

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
    output logic             axi_slv_w_ready

);

    localparam B_OKAY   = 2'b00;

    typedef struct packed {
        id_t   id  ;
        data_t data;
        resp_t resp;
        logic  last;
    } r_t;

    typedef byte unsigned dpi_data[DATA_WIDTH/$bits(byte)];
    typedef byte unsigned dpi_strb[STRB_WIDTH/$bits(byte)];

    typedef logic[$clog2(R_FIFO_DEPTH  )-1:0] r_queue_idx_t;
    typedef logic[$clog2(R_FIFO_DEPTH+1)-1:0] r_queue_ptr_t;
    localparam r_queue_ptr_t RFD = r_queue_ptr_t'(R_FIFO_DEPTH);

    typedef int     unsigned UI;
    typedef byte    unsigned UB;
    typedef longint unsigned UL;

    import "DPI-C" context function chandle axi_sw_new(chandle endpoint_p, byte unsigned poll, int unsigned data_width, string tag, int unsigned r_q_max, int unsigned r_q_ptr_max);
    import "DPI-C" function void axi_sw_aw(chandle axi_sw_p, int unsigned id, longint unsigned addr, byte unsigned len, byte unsigned size, byte unsigned burst, byte unsigned atop);
    import "DPI-C" function void axi_sw_ar(chandle axi_sw_p, int unsigned id, longint unsigned addr, byte unsigned len, byte unsigned size, byte unsigned burst);
    import "DPI-C" function void axi_sw_w(chandle axi_sw_p, dpi_data data, dpi_strb strb, byte unsigned last);
    import "DPI-C" function void axi_sw_r_ptr(chandle axi_sw_p, int unsigned r_ptr);
    import "DPI-C" context function void axi_sw_r_poll(chandle axi_sw_p);

    if ($bits(r_queue_ptr_t) > $bits(int unsigned))
        $error("not enough bits");

    if ($bits(addr_t) > $bits(longint unsigned))
        $error("not enough bits");

    if ($bits(id_t) > $bits(int unsigned))
        $error("not enough bits");

    r_t r_queue[R_FIFO_DEPTH];
    r_queue_ptr_t  r_queue_size, r_queue_wptr='0, r_queue_rptr;

    logic b_queue_full     , b_queue_empty     ;
    logic w_last_queue_full, w_last_queue_empty;
    logic r_queue_full     , r_queue_empty     ;

    chandle axi_sw_p;
    bit r_poll;
    string prog;
    initial begin
        if (!$value$plusargs("axi_sw_r_poll:%d", r_poll)) begin
            r_poll = POLL_DEFAULT;
        end
        // can be generalized to non-system model
        axi_sw_p = axi_sw_new(sysmod_pkg::get(SYSMOD_NUM), byte'(r_poll), DATA_WIDTH, tag, R_FIFO_DEPTH, 1 << $bits(r_queue_ptr_t));
    end

    function automatic axi_sw_r (int unsigned id, dpi_data data, byte unsigned last);
        data_t d;
        r_t r;
        assert(!r_queue_full);
        // stream pack unsupported by verilator
        for (int i = 0; i < $size(dpi_data); i++) begin
            d[8*i +: 8] = data[i];
        end
        r = '{id: id_t'(id), data: data_t'(d), resp: B_OKAY, last: 1'(last)};
        r_queue[r_queue_idx_t'(r_queue_wptr % RFD)] = r;
        r_queue_wptr++;
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

    always_ff @(posedge clk) begin
        if (reset_n) begin
            if (axi_slv_r_valid && axi_mst_r_ready) begin
                automatic r_queue_ptr_t n = r_queue_rptr + r_queue_ptr_t'(1);
                r_queue_rptr <= n;
                r_queue_rptr_incremented <= '1;
            end else begin
                r_queue_rptr_incremented <= '0;
            end
            if (r_queue_rptr_incremented) begin
                axi_sw_r_ptr(axi_sw_p, int'(r_queue_rptr));
            end
            if(r_poll) begin
                axi_sw_r_poll(axi_sw_p);
            end
        end else begin
            r_queue_rptr <= '0;
        end
    end

    assign axi_slv_aw_ready = !b_queue_full;
    assign axi_slv_ar_ready = '1;
    assign axi_slv_w_ready  = !axi_mst_w_last || !w_last_queue_full;
    assign axi_slv_b_valid  = !b_queue_empty && !w_last_queue_empty;
    assign axi_slv_b_resp   = B_OKAY;
    assign axi_slv_r_valid  = !r_queue_empty;

    axi_sw_fifo #(
        .D         (1),
        .T         (id_t)
    ) b_queue (
        .clk         (clk                                 ),
        .reset_n     (reset_n                             ),
        .full        (b_queue_full                        ),
        .empty       (b_queue_empty                       ),
        .d           (axi_mst_aw_id                       ),
        .push        (axi_mst_aw_valid && axi_slv_aw_ready),
        .q           (axi_slv_b_id                        ),
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

    always_ff @(posedge clk) begin
        if (reset_n) begin
            if (axi_mst_aw_valid && axi_slv_aw_ready) begin
                axi_sw_aw(axi_sw_p, UI'(axi_mst_aw_id), UL'(axi_mst_aw_addr), UB'(axi_mst_aw_len), UB'(axi_mst_aw_size), UB'(axi_mst_aw_burst), UB'(axi_mst_aw_atop));
            end
            if (axi_mst_ar_valid && axi_slv_ar_ready) begin
                axi_sw_ar(axi_sw_p, UI'(axi_mst_ar_id), UL'(axi_mst_ar_addr), UB'(axi_mst_ar_len), UB'(axi_mst_ar_size), UB'(axi_mst_ar_burst));
            end
            if (axi_mst_w_valid && axi_slv_w_ready) begin
                automatic dpi_data data;
                automatic dpi_strb strb;
                // streaming unpack unsupported by verilator
                //data = {>>byte {axi_mst_w_data}};
                //strb = {>>byte {axi_mst_w_strb}};
                for (int i = 0; i < $size(dpi_data);  i++) begin
                    data[i] = axi_mst_w_data[8 * i +: 8];
                    strb[i/8][i%8] = axi_mst_w_strb[i];
                end
                axi_sw_w(axi_sw_p, data, strb, UB'(axi_mst_w_last));
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
