// can't call a function from a function in DPI export on palladium
`define AXI_SW_DPI_FIFO_PUSH(name, D, data, rptr)              \
    if (1) begin                                               \
        typedef logic[$clog2(D+1)-1:0] ptr_t;                  \
        typedef logic[$clog2(D+0)-1:0] idx_t;                  \
        localparam ptr_t PD = ptr_t'(D);                       \
        `ifndef IMMEDIATE_ASSERTIONS_IN_DPI_UNSUPPORTED        \
        assert(name``_wptr_nxt - rptr != PD) else              \
            $error("fifo is full");                            \
        assert(reset_n) else $error("called in reset");        \
        `endif                                                 \
        if (reset_n) begin                                     \
            name``_q[idx_t'(name``_wptr_nxt % PD)] = data;     \
            name``_wptr_nxt++;                                 \
        end                                                    \
    end                                                        \

`define AXI_SW_DPI_FIFO_RESET(name)                        \
    function void name``_reset();                          \
        name``_wptr_nxt = '0;                              \
    endfunction                                            \
    export "DPI-C" function name``_reset;

`define AXI_SW_DPI_FIFO(name, T, DEPTH, clk, sys_reset, reset_n, pop, popped, empty, out, rptr) \
    localparam type         name``_idx_t = logic[$clog2(DEPTH  )-1:0];         \
    localparam type         name``_ptr_t = logic[$clog2(DEPTH+1)-1:0];         \
    localparam name``_ptr_t name``_D     = name``_ptr_t'(DEPTH);               \
                                                                               \
    T name``_q[DEPTH];                                                         \
    name``_ptr_t name``_size, name``_wptr, name``_wptr_nxt;                    \
                                                                               \
    assign name``_size  = name``_wptr - rptr;                                  \
//  assign full  = name``_size == name``_D;                                    \
    assign empty = name``_size == '0;                                          \
                                                                               \
    always @(posedge clk) begin                                                \
        automatic logic popped_nxt = (reset_n) ? (pop): '0;                    \
        popped <=  popped_nxt;                                                 \
        rptr          <= !(sys_reset) ? rptr + name``_ptr_t'(popped_nxt) : '0; \
        name``_wptr   <= name``_wptr_nxt;                                      \
    end                                                                        \
                                                                               \
    assign out = name``_q[name``_idx_t'(rptr % name``_D)];                     \
    `AXI_SW_DPI_FIFO_RESET(name)

module axi_sw #(

    parameter int unsigned ADDR_WIDTH = 32'd0,
    parameter int unsigned DATA_WIDTH = 32'd0,
    parameter int unsigned STRB_WIDTH = DATA_WIDTH / 8,
    parameter int unsigned ID_WIDTH   = 32'd0,

    parameter int unsigned TOPO_ID    =    -1,
    parameter int NUM                 =    -1,
    parameter string tag = "notag",
    parameter int unsigned R_Q_MAX    = 32'd0,

    parameter type addr_t   = logic [ADDR_WIDTH-1:0],
    parameter type data_t   = logic [DATA_WIDTH-1:0],
    parameter type id_t     = logic [ID_WIDTH  -1:0],
    parameter type strb_t   = logic [STRB_WIDTH-1:0],

    parameter type burst_t  = logic [1:0],
    parameter type atop_t   = logic [5:0],
    parameter type resp_t   = logic [1:0],
    parameter type len_t    = logic [7:0],
    parameter type size_t   = logic [2:0],
    `RV_TESTER_TRANSACTIONS_AXI_SW_OUTPUT_PARAMS

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
    `RV_TESTER_TRANSACTIONS_AXI_SW_OUTPUT_PORTS

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

    typedef int     unsigned UI;
    typedef byte    unsigned UB;
    typedef longint unsigned UL;

    logic b_queue_full     , b_queue_empty     ;
    logic w_last_queue_full, w_last_queue_empty;
    logic r_queue_empty     ;

    import "DPI-C" context function void axi_sw_set_scope(int unsigned location);

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

    logic r_queue_rptr_incremented;
    logic [$clog2(R_Q_MAX+1)-1:0] r_queue_rptr;
    r_t r;
    `AXI_SW_DPI_FIFO(axi_sw_r, r_t, R_Q_MAX, clk, sys_reset, reset_n, axi_slv_r_valid && axi_mst_r_ready, r_queue_rptr_incremented, r_queue_empty, r, r_queue_rptr)

    `define AXI_SW_R_SIZED(S)                                                                                        \
        function void axi_sw_r_``S (int unsigned id, byte unsigned resp, byte unsigned data[S], byte unsigned last); \
            data_t d;                                                                                                \
            r_t rd;                                                                                                  \
            if ($size(dpi_data) == S) begin                                                                          \
                // stream pack unsupported by verilator                                                              \
                for (int i = 0; i < S; i++) begin                                                                    \
                    d[8*i +: 8] = data[i];                                                                           \
                end                                                                                                  \
                rd = '{id: id_t'(id), data: data_t'(d), resp: 2'(resp), last: 1'(last)};                             \
                `AXI_SW_DPI_FIFO_PUSH(axi_sw_r,R_Q_MAX,rd,r_queue_rptr)                                              \
            end                                                                                                      \
        endfunction                                                                                                  \
        export "DPI-C" function axi_sw_r_``S;

    `AXI_SW_R_SIZED(8)
    `AXI_SW_R_SIZED(64)

    `undef AXI_SW_R_SIZED

    always_comb begin
        axi_slv_r_id    = r.id  ;
        axi_slv_r_data  = r.data;
        axi_slv_r_resp  = r.resp;
        axi_slv_r_last  = r.last;
    end

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
            if (r_queue_rptr_incremented) begin
                r_q_ptrs[0].valid         <= '1 & (location != cvm_topology::nil);
                r_q_ptrs[0].data.location <= location;
                r_q_ptrs[0].data.r_ptr    <= r_queue_rptr;
            end
        end

        if (reset_n) begin
            if (axi_mst_w_valid && axi_slv_w_ready) begin
                ws[0].valid          <= '1 & (location != cvm_topology::nil);
                ws[0].data.location  <= location;
                ws[0].data.data      <= axi_mst_w_data;
                ws[0].data.strb      <= axi_mst_w_strb;
                ws[0].data.last      <= axi_mst_w_last;
            end
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

module axi_sw_mst #(

    parameter int unsigned ADDR_WIDTH = 32'd0,
    parameter int unsigned DATA_WIDTH = 32'd0,
    parameter int unsigned STRB_WIDTH = DATA_WIDTH / 8,
    parameter int unsigned ID_WIDTH   = 32'd0,

    parameter int unsigned TOPO_ID    =    -1,
    parameter int NUM                 =    -1,
    parameter string tag = "notag",
    parameter int unsigned AR_Q_MAX   = 32'd0,
    parameter int unsigned AW_Q_MAX   = 32'd0,
    parameter int unsigned W_Q_MAX    = 32'd0,

    parameter type addr_t   = logic [ADDR_WIDTH-1:0],
    parameter type data_t   = logic [DATA_WIDTH-1:0],
    parameter type id_t     = logic [ID_WIDTH  -1:0],
    parameter type strb_t   = logic [STRB_WIDTH-1:0],

    parameter type burst_t  = logic [1:0],
    parameter type atop_t   = logic [5:0],
    parameter type resp_t   = logic [1:0],
    parameter type len_t    = logic [7:0],
    parameter type size_t   = logic [2:0],

    `RV_TESTER_TRANSACTIONS_AXI_SW_MST_OUTPUT_PARAMS

)(

    input  clk,
    input  reset_n,
    input  sys_reset,

    output logic             axi_mst_ar_valid,
    output id_t              axi_mst_ar_id,
    output addr_t            axi_mst_ar_addr,
    output len_t             axi_mst_ar_len,
    output size_t            axi_mst_ar_size,
    output burst_t           axi_mst_ar_burst,
    output logic             axi_mst_ar_lock,

    output logic             axi_mst_aw_valid,
    output id_t              axi_mst_aw_id,
    output addr_t            axi_mst_aw_addr,
    output len_t             axi_mst_aw_len,
    output size_t            axi_mst_aw_size,
    output burst_t           axi_mst_aw_burst,
    output atop_t            axi_mst_aw_atop,
    output logic             axi_mst_aw_lock,

    output logic             axi_mst_w_valid,
    output data_t            axi_mst_w_data,
    output strb_t            axi_mst_w_strb,
    output logic             axi_mst_w_last,

    output logic             axi_mst_b_ready,
    output logic             axi_mst_r_ready,

    input  logic             axi_slv_b_valid,
    input  id_t              axi_slv_b_id,
    input  resp_t            axi_slv_b_resp,

    input  logic             axi_slv_r_valid,
    input  id_t              axi_slv_r_id,
    input  data_t            axi_slv_r_data,
    input  resp_t            axi_slv_r_resp,
    input  logic             axi_slv_r_last,

    input  logic             axi_slv_aw_ready,
    input  logic             axi_slv_ar_ready,
    input  logic             axi_slv_w_ready,
    `RV_TESTER_TRANSACTIONS_AXI_SW_MST_OUTPUT_PORTS

);
    typedef byte unsigned dpi_data[DATA_WIDTH/$bits(byte)];
    typedef byte unsigned dpi_strb[STRB_WIDTH/$bits(byte)];

    typedef struct packed {
        id_t              id;
        addr_t            addr;
        len_t             len;
        size_t            size;
        burst_t           burst;
        logic             lock;
    } ar_t;

    typedef struct packed {
        id_t              id;
        addr_t            addr;
        len_t             len;
        size_t            size;
        burst_t           burst;
        atop_t            atop;
        logic             lock;
    } aw_t;

    typedef struct packed {
        data_t            data;
        strb_t            strb;
        logic             last;
    } w_t;


    import "DPI-C" context function void axi_sw_mst_set_scope(int unsigned location);

    int unsigned location = cvm_topology::nil;
    always @(posedge clk) begin
        if (sys_reset) begin
            /* verilator lint_off BLKSEQ */
                // FIXME add a reset for the axi xtor
            location = cvm_topology::get_location(TOPO_ID, NUM);
            if (location != cvm_topology::nil) begin
                axi_sw_mst_set_scope(location);
            end
            /* verilator lint_on BLKSEQ */
        end
    end

    logic ar_queue_rptr_incremented, ar_queue_empty;
    logic [$clog2(AR_Q_MAX+1)-1:0] ar_queue_rptr;
    ar_t ar;

    `AXI_SW_DPI_FIFO(axi_sw_mst_ar, ar_t, AR_Q_MAX, clk, sys_reset, reset_n, axi_slv_ar_ready && axi_mst_ar_valid, ar_queue_rptr_incremented, ar_queue_empty, ar, ar_queue_rptr)

    function void axi_sw_mst_ar (int unsigned id, longint unsigned addr, byte unsigned len, byte unsigned size, byte unsigned burst, byte unsigned lock);
        ar_t p;
        p = '{id: id_t'(id), addr: addr_t'(addr), len: len_t'(len), size: size_t'(size), burst: burst_t'(burst), lock: (1)'(lock)};
        `AXI_SW_DPI_FIFO_PUSH(axi_sw_mst_ar,AR_Q_MAX,p,ar_queue_rptr)
    endfunction
    export "DPI-C" function axi_sw_mst_ar;

    always_comb begin
        axi_mst_ar_valid = !ar_queue_empty;
        axi_mst_ar_id    = ar.id;
        axi_mst_ar_addr  = ar.addr;
        axi_mst_ar_len   = ar.len;
        axi_mst_ar_size  = ar.size;
        axi_mst_ar_burst = ar.burst;
        axi_mst_ar_lock  = ar.lock;
    end

    logic aw_queue_rptr_incremented, aw_queue_empty;
    logic [$clog2(AW_Q_MAX+1)-1:0] aw_queue_rptr;
    aw_t aw;

    `AXI_SW_DPI_FIFO(axi_sw_mst_aw, aw_t, AW_Q_MAX, clk, sys_reset, reset_n, axi_slv_aw_ready && axi_mst_aw_valid, aw_queue_rptr_incremented, aw_queue_empty, aw, aw_queue_rptr)

    function void axi_sw_mst_aw (int unsigned id, longint unsigned addr, byte unsigned len, byte unsigned size, byte unsigned burst, byte unsigned atop, byte unsigned lock);
        aw_t p;
        p = '{id: id_t'(id), addr: addr_t'(addr), len: len_t'(len), size: size_t'(size), burst: burst_t'(burst), atop: atop_t'(atop), lock: 1'(lock)};
        `AXI_SW_DPI_FIFO_PUSH(axi_sw_mst_aw,AW_Q_MAX,p,aw_queue_rptr)
    endfunction
    export "DPI-C" function axi_sw_mst_aw;

    always_comb begin
        axi_mst_aw_valid = !aw_queue_empty;
        axi_mst_aw_id    = aw.id;
        axi_mst_aw_addr  = aw.addr;
        axi_mst_aw_len   = aw.len;
        axi_mst_aw_size  = aw.size;
        axi_mst_aw_burst = aw.burst;
        axi_mst_aw_atop  = aw.atop;
        axi_mst_aw_lock  = aw.lock;
    end

    logic w_queue_rptr_incremented, w_queue_empty;
    logic [$clog2(W_Q_MAX+1)-1:0] w_queue_rptr;
    w_t w;

    `AXI_SW_DPI_FIFO(axi_sw_mst_w, w_t, W_Q_MAX, clk, sys_reset, reset_n, axi_slv_w_ready && axi_mst_w_valid, w_queue_rptr_incremented, w_queue_empty, w, w_queue_rptr)

`define AXI_SW_MST_W(data, strb, last)                    \
        w_t p;                                            \
        data_t d;                                         \
        strb_t s;                                         \
        if ($size(data) != DATA_WIDTH/$size(byte)) begin  \
            $error();                                     \
        end                                               \
        for (int i = 0; i < $size(dpi_data); i++) begin   \
            d[8*i +: 8] = data[i];                        \
        end                                               \
        for (int i = 0; i < $size(dpi_strb); i++) begin   \
            s[8*i +: 8] = strb[i];                        \
        end                                               \
        p = '{data: d, strb: s, last: (1)'(last)};        \
        `AXI_SW_DPI_FIFO_PUSH(axi_sw_mst_w,W_Q_MAX,p, w_queue_rptr)

    function void axi_sw_mst_w_64 (byte unsigned data[64], byte unsigned strb[8], byte unsigned last);
        `AXI_SW_MST_W(data, strb, last)
    endfunction
    export "DPI-C" function axi_sw_mst_w_64;

    function void axi_sw_mst_w_8 (byte unsigned data[8], byte unsigned strb[1], byte unsigned last);
        `AXI_SW_MST_W(data, strb, last)
    endfunction
    export "DPI-C" function axi_sw_mst_w_8;

    always_comb begin
        axi_mst_w_valid = !w_queue_empty;
        axi_mst_w_data  = w.data;
        axi_mst_w_strb  = w.strb;
        axi_mst_w_last  = w.last;
    end

    assign axi_mst_b_ready = '1;
    assign axi_mst_r_ready = '1;

    always @(posedge clk) begin
        if (reset_n) begin
            ar_q_ptrs[0].valid          <= ar_queue_rptr_incremented;
            ar_q_ptrs[0].data.location  <= location;
            ar_q_ptrs[0].data.ar_ptr    <= ar_queue_rptr;

            aw_q_ptrs[0].valid          <= aw_queue_rptr_incremented;
            aw_q_ptrs[0].data.location  <= location;
            aw_q_ptrs[0].data.aw_ptr    <= aw_queue_rptr;

            w_q_ptrs[0].valid         <= w_queue_rptr_incremented;
            w_q_ptrs[0].data.location <= location;
            w_q_ptrs[0].data.w_ptr    <= w_queue_rptr;

            bs[0].valid         <= axi_slv_b_valid && axi_mst_b_ready;
            bs[0].data.location <= location;
            bs[0].data.id       <= axi_slv_b_id;
            bs[0].data.resp     <= axi_slv_b_resp;

            rs[0].valid         <= axi_slv_r_valid && axi_mst_r_ready;
            rs[0].data.location <= location;
            rs[0].data.id       <= axi_slv_r_id;
            rs[0].data.data     <= axi_slv_r_data;
            rs[0].data.resp     <= axi_slv_r_resp;
            rs[0].data.last     <= axi_slv_r_last;
        end
    end

endmodule

`undef AXI_SW_DPI_FIFO_PUSH
`undef AXI_SW_DPI_FIFO_RESET
`undef AXI_SW_DPI_FIFO
