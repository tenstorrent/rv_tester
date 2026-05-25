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
    end

`define AXI_SW_DPI_FIFO_RESET(name)                        \
    function void name``_reset();                          \
        name``_wptr_nxt = '0;                              \
    endfunction                                            \
    export "DPI-C" function name``_reset;

`define AXI_SW_DPI_FIFO(name, T, DEPTH, clk, sys_reset, reset_n, update_rptr, rptr_updated, empty, out, rptr) \
    localparam type         name``_idx_t = logic[$clog2(DEPTH  )-1:0];               \
    localparam type         name``_ptr_t = logic[$clog2(DEPTH+1)-1:0];               \
    localparam name``_ptr_t name``_D     = name``_ptr_t'(DEPTH);                     \
                                                                                     \
    T name``_q[DEPTH];                                                               \
    name``_ptr_t name``_size, name``_wptr, name``_wptr_nxt;                          \
                                                                                     \
    assign name``_size  = name``_wptr - rptr;                                        \
//  assign full  = name``_size == name``_D;                                          \
    assign empty = name``_size == '0;                                                \
                                                                                     \
    always @(posedge clk) begin                                                      \
        automatic logic rptr_updated_nxt = (reset_n) ? (update_rptr): '0;            \
        rptr_updated <=  rptr_updated_nxt | sys_reset;                               \
        rptr          <= !(sys_reset) ? rptr + name``_ptr_t'(rptr_updated_nxt) : '0; \
        /* verilator lint_off BLKSEQ */                                              \
        if (sys_reset) name``_wptr_nxt = '0;                                         \
        /* verilator lint_on BLKSEQ */                                               \
        name``_wptr   <= name``_wptr_nxt;                                            \
    end                                                                              \
                                                                                     \
    assign out = name``_q[name``_idx_t'(rptr % name``_D)];                           \
    `AXI_SW_DPI_FIFO_RESET(name)

module axi_sw #(

    parameter int unsigned ADDR_WIDTH = 32'd0,
    parameter int unsigned DATA_WIDTH = 32'd0,
    parameter int unsigned STRB_WIDTH = DATA_WIDTH / 8,
    parameter int unsigned ID_WIDTH   = 32'd0,

    parameter int unsigned LOCATION   =     0,
    parameter string tag = "notag",
    parameter int unsigned R_Q_MAX    = 32'd0,
    parameter int unsigned B_Q_MAX    = 32'd0,

    parameter type addr_t   = logic [ADDR_WIDTH-1:0],
    parameter type data_t   = logic [DATA_WIDTH-1:0],
    parameter type id_t     = logic [ID_WIDTH  -1:0],
    parameter type strb_t   = logic [STRB_WIDTH-1:0],

    parameter type burst_t  = logic [1:0],
    parameter type resp_t   = logic [1:0],
    parameter type len_t    = logic [7:0],
    parameter type size_t   = logic [2:0],
    parameter type cache_t  = logic [3:0],
    parameter type prot_t   = logic [2:0],
    parameter type qos_t    = logic [3:0],
    parameter type region_t = logic [3:0],
    parameter type atop_t   = logic [5:0],
    parameter type user_t   = logic [0:0],
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
    input  cache_t           axi_mst_ar_cache,
    input  prot_t            axi_mst_ar_prot,
    input  qos_t             axi_mst_ar_qos,
    input  region_t          axi_mst_ar_region,
    input  atop_t            axi_mst_ar_atop,
    input  user_t            axi_mst_ar_user,

    input  logic             axi_mst_aw_valid,
    input  id_t              axi_mst_aw_id,
    input  addr_t            axi_mst_aw_addr,
    input  len_t             axi_mst_aw_len,
    input  size_t            axi_mst_aw_size,
    input  burst_t           axi_mst_aw_burst,
    input  logic             axi_mst_aw_lock,
    input  cache_t           axi_mst_aw_cache,
    input  prot_t            axi_mst_aw_prot,
    input  qos_t             axi_mst_aw_qos,
    input  region_t          axi_mst_aw_region,
    input  atop_t            axi_mst_aw_atop,
    input  user_t            axi_mst_aw_user,

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
        shortint unsigned latency;
    } r_t;

    typedef struct packed {
        id_t   id  ;
        resp_t resp;
        shortint unsigned latency;
    } b_t;

    typedef byte unsigned dpi_data[DATA_WIDTH/$bits(byte)];
    typedef byte unsigned dpi_strb[STRB_WIDTH/$bits(byte)];

    typedef int     unsigned UI;
    typedef byte    unsigned UB;
    typedef longint unsigned UL;

    logic b_queue_empty     ;
    logic r_queue_empty     ;

    import "DPI-C" context function void axi_sw_reset_ptrs(int unsigned location);

    always @(posedge clk) begin
        if (sys_reset) begin

            /* verilator lint_off BLKSEQ */
            if (LOCATION != cvm_topology::nil) begin
                // FIFO wptr_nxt reset is handled inside AXI_SW_DPI_FIFO macro on
                // sys_reset; ZeBu/ISC disallows DUT calls to TB_EXPORT (so the
                // exported axi_sw_*_reset() functions cannot be invoked here).
                axi_sw_reset_ptrs(LOCATION);
                cvm_registry::set_scope(LOCATION);
            end
            /* verilator lint_on BLKSEQ */
        end
    end

    logic r_queue_rptr_incremented;
    logic [$clog2(R_Q_MAX+1)-1:0] r_queue_rptr;
    r_t r;
    `AXI_SW_DPI_FIFO(axi_sw_r, r_t, R_Q_MAX, clk, sys_reset, reset_n, axi_slv_r_valid && axi_mst_r_ready, r_queue_rptr_incremented, r_queue_empty, r, r_queue_rptr)

    `define AXI_SW_R_SIZED(S)                                                                                                                   \
        function void axi_sw_r_``S (int unsigned id, byte unsigned resp, byte unsigned data[S], byte unsigned last, shortint unsigned latency); \
            data_t d;                                                                                                                           \
            r_t rd;                                                                                                                             \
            if ($size(dpi_data) == S) begin                                                                                                     \
                // stream pack unsupported by verilator                                                                                         \
                for (int i = 0; i < S; i++) begin                                                                                               \
                    d[8*i +: 8] = data[i];                                                                                                      \
                end                                                                                                                             \
                rd = '{id: id_t'(id), data: data_t'(d), resp: 2'(resp), last: 1'(last), latency: 16'(latency)};                                 \
                `AXI_SW_DPI_FIFO_PUSH(axi_sw_r,R_Q_MAX,rd,r_queue_rptr)                                                                         \
            end                                                                                                                                 \
        endfunction                                                                                                                             \
        export "DPI-C" function axi_sw_r_``S;

    `AXI_SW_R_SIZED(8)
    `AXI_SW_R_SIZED(32)
    `AXI_SW_R_SIZED(64)

    `undef AXI_SW_R_SIZED

    always_comb begin
      axi_slv_r_id    = 'X;
      axi_slv_r_data  = 'X;
      axi_slv_r_resp  = 'X;
      axi_slv_r_last  = 'X;

      if (axi_slv_r_valid) begin
        axi_slv_r_id    = r.id  ;
        axi_slv_r_data  = r.data;
        axi_slv_r_resp  = r.resp;
        axi_slv_r_last  = r.last;
      end
    end

    logic fast_b_response, fast_b_queue_full, fast_b_queue_empty;
    id_t fast_axi_slv_b_id;
    logic [1:0] fast_axi_slv_b_resp;
    rv_tester_fifo #(
        .D         (1),
        .T         (logic[$bits(id_t)+2-1:0])
    ) fast_b_queue (
        .clk         (clk                                 ),
        .reset_n     (reset_n                             ),
        .full        (fast_b_queue_full                        ),
        .empty       (fast_b_queue_empty                       ),
        .d           ({axi_mst_aw_id, axi_mst_aw_lock? RESP_EXOKAY : RESP_OKAY}    ),
        .push        (axi_mst_aw_valid && axi_slv_aw_ready && fast_b_response),
        .q           ({fast_axi_slv_b_id , fast_axi_slv_b_resp}    ),
        .pop         (axi_slv_b_valid && axi_mst_b_ready && fast_b_response)
    );

    logic w_last_queue_full, w_last_queue_empty;
    rv_tester_fifo #(
        .D         (1),
        .T         (logic)
    ) w_last_queue (
        .clk         (clk                                                 ),
        .reset_n     (reset_n                                             ),
        .full        (w_last_queue_full                                   ),
        .empty       (w_last_queue_empty                                  ),
        .d           (1'b1                                                ),
        .push        (axi_mst_w_valid && axi_slv_w_ready && axi_mst_w_last && fast_b_response),
        .q           (                                                    ),
        .pop         (axi_slv_b_valid && axi_mst_b_ready && fast_b_response)
    );

    always_comb begin
      axi_slv_b_id = 'X;
      axi_slv_b_resp = 'X;

      if (axi_slv_b_valid) begin
        axi_slv_b_id    = fast_b_response ? fast_axi_slv_b_id : b.id  ;
        axi_slv_b_resp  = fast_b_response ? fast_axi_slv_b_resp : b.resp;
      end
    end

    `define AXI_SW_TOGGLE_READY(tx)                                                  \
      int unsigned lfsr_seed_``tx = '0;                                              \
      int unsigned lfsr_mask_``tx = '0;                                              \
      int unsigned lfsr_out_``tx;                                                    \
      logic tx``_ready_random;                                                       \
      assign tx``_ready_random = (lfsr_out_``tx & lfsr_mask_``tx``) == 32'd0;        \
      cvm_lfsr #(                                                                    \
          .WIDTH(32'd32),                                                            \
          .NUM_TAPS(32'd4),                                                          \
          .TAPS({32'd31, 32'd6, 32'd5, 32'd1})                                       \
      ) ready_toggle_lfsr_``tx (                                                     \
          .clk(clk),                                                                 \
          .rst(sys_reset),                                                           \
          .seed(lfsr_seed_``tx``),                                                   \
          .step(1'b1),                                                               \
          .out_state(lfsr_out_``tx``)                                                \
      );                                                                             \
      always @(posedge clk) begin                                                    \
          if (sys_reset) begin                                                       \
              /* verilator lint_off BLKSEQ */                                        \
              lfsr_seed_``tx = cvm_plusargs::get_int({"axi_sw_lfsr_seed_", `"tx`", "_rdy"}); \
              lfsr_mask_``tx = cvm_plusargs::get_int({"axi_sw_lfsr_mask_", `"tx`", "_rdy"}); \
              /* verilator lint_on BLKSEQ */                                         \
          end                                                                        \
      end

    `AXI_SW_TOGGLE_READY(aw)
    `AXI_SW_TOGGLE_READY(ar)
    `AXI_SW_TOGGLE_READY(w)

    `undef AXI_SW_TOGGLE_READY

    logic ar_history_full;
    logic aw_history_full;
    logic read_latency_requirement_met;

    assign axi_slv_aw_ready = fast_b_response ? !fast_b_queue_full : (aw_ready_random & !aw_history_full);
    assign axi_slv_ar_ready = ar_ready_random & !ar_history_full;
    assign axi_slv_w_ready  = fast_b_response ? (!axi_mst_w_last || !w_last_queue_full) : (w_ready_random & !aw_history_full);

    shortint unsigned axi_slv_b_delay = '0;
    shortint unsigned axi_slv_r_delay = '0;

    assign axi_slv_b_valid  = reset_n ? fast_b_response ? (!fast_b_queue_empty && !w_last_queue_empty) : (!b_queue_empty && (axi_slv_b_delay == 0)): '0;
    assign axi_slv_r_valid  = reset_n ? (!r_queue_empty && read_latency_requirement_met && (axi_slv_r_delay == 0)) : '0;

    logic b_queue_rptr_incremented;
    logic [$clog2(B_Q_MAX+1)-1:0] b_queue_rptr;
    b_t b;
    `AXI_SW_DPI_FIFO(axi_sw_b, b_t, B_Q_MAX, clk, sys_reset, reset_n, axi_slv_b_valid && axi_mst_b_ready, b_queue_rptr_incremented, b_queue_empty, b, b_queue_rptr)

    function automatic void axi_sw_b (int unsigned id, byte unsigned resp, shortint unsigned latency);
      b_t bd = '{id: id_t'(id), resp: 2'(resp), latency: 16'(latency)};
      `AXI_SW_DPI_FIFO_PUSH(axi_sw_b,B_Q_MAX,bd,b_queue_rptr);
    endfunction
    export "DPI-C" function axi_sw_b;

    always @(posedge clk) begin
        axi_slv_b_delay <= (axi_slv_b_delay != 0) ? axi_slv_b_delay - 1 : b.latency;
        axi_slv_r_delay <= (axi_slv_r_delay != 0) ? axi_slv_r_delay - 1 : r.latency;
    end

    logic [64-1:0] clocks;
    always_ff @(posedge clk) begin
        if (!reset_n) begin
            clocks <= '0;
        end else begin
            clocks <= clocks + 64'(1);
        end
    end

    always @(posedge clk) begin
        aws[0].valid      <= '0;
        ars[0].valid      <= '0;
        ws[0].valid       <= '0;
        r_q_ptrs[0].valid <= '0;
        b_q_ptrs[0].valid <= '0;

        if (reset_n) begin
            if (r_queue_rptr_incremented) begin
                r_q_ptrs[0].valid         <= '1 & (LOCATION != cvm_topology::nil);
                r_q_ptrs[0].data.location <= LOCATION;
                r_q_ptrs[0].data.r_ptr    <= r_queue_rptr;
                r_q_ptrs[0].data.clock    <= clocks;
            end
            if (b_queue_rptr_incremented) begin
                b_q_ptrs[0].valid         <= '1 & (LOCATION != cvm_topology::nil);
                b_q_ptrs[0].data.location <= LOCATION;
                b_q_ptrs[0].data.r_ptr    <= b_queue_rptr;
                b_q_ptrs[0].data.clock    <= clocks;
            end
        end

        if (reset_n) begin
            if (axi_mst_w_valid && axi_slv_w_ready) begin
                ws[0].valid          <= '1 & (LOCATION != cvm_topology::nil);
                ws[0].data.location  <= LOCATION;
                ws[0].data.data      <= axi_mst_w_data;
                ws[0].data.strb      <= axi_mst_w_strb;
                ws[0].data.last      <= axi_mst_w_last;
            end
            if (axi_mst_aw_valid && axi_slv_aw_ready) begin
                aws[0].valid          <= '1 & (LOCATION != cvm_topology::nil);
                aws[0].data.location  <= LOCATION;
                aws[0].data.id        <= axi_mst_aw_id;
                aws[0].data.addr      <= axi_mst_aw_addr;
                aws[0].data.len       <= axi_mst_aw_len;
                aws[0].data.size      <= axi_mst_aw_size;
                aws[0].data.burst     <= axi_mst_aw_burst;
                aws[0].data.lock      <= axi_mst_aw_lock;
                aws[0].data.atop      <= axi_mst_aw_atop;
                aws[0].data.cache     <= axi_mst_aw_cache;
                aws[0].data.prot      <= axi_mst_aw_prot;
                aws[0].data.qos       <= axi_mst_aw_qos;
                aws[0].data.region    <= axi_mst_aw_region;
                aws[0].data.atop      <= axi_mst_aw_atop;
                aws[0].data.user      <= axi_mst_aw_user;


            end
            if (axi_mst_ar_valid && axi_slv_ar_ready) begin
                ars[0].valid          <= '1 & (LOCATION != cvm_topology::nil);
                ars[0].data.location  <= LOCATION;
                ars[0].data.id        <= axi_mst_ar_id;
                ars[0].data.addr      <= axi_mst_ar_addr;
                ars[0].data.len       <= axi_mst_ar_len;
                ars[0].data.size      <= axi_mst_ar_size;
                ars[0].data.burst     <= axi_mst_ar_burst;
                ars[0].data.lock      <= axi_mst_ar_lock;
                ars[0].data.cache     <= axi_mst_ar_cache;
                ars[0].data.prot      <= axi_mst_ar_prot;
                ars[0].data.qos       <= axi_mst_ar_qos;
                ars[0].data.region    <= axi_mst_ar_region;
                ars[0].data.user      <= axi_mst_ar_user;
            end
        end
    end

    localparam int CW = 16;
    int unsigned read_latency                   = '0;
    int unsigned read_latency_timeout_threshold = '0;
    int unsigned read_latency_fifo_threshold    = '0;
    bit          read_latency_fixed             = '0;
    int unsigned reorder_latency_timeout        = '0;
    bit          reorder_window                 = '0;
    int unsigned axi_sw_read_latency_max        = '0;
    int unsigned axi_sw_read_latency_fixed      = '0;
    always @(posedge clk) begin
        if (sys_reset) begin
            /* verilator lint_off BLKSEQ */
            axi_sw_read_latency_max        = cvm_plusargs::get_int("axi_sw_read_latency_max");
            axi_sw_read_latency_fixed      = cvm_plusargs::get_int("axi_sw_read_latency_fixed");
            read_latency_timeout_threshold = cvm_plusargs::get_int("axi_sw_read_latency_timeout_threshold");
            read_latency_fifo_threshold    = cvm_plusargs::get_int("axi_sw_read_latency_fifo_threshold");
            reorder_latency_timeout        = cvm_plusargs::get_int("axi_sw_reorder_timeout");
            reorder_window                 = cvm_plusargs::get_int("axi_sw_reorder_window") != 0;
            fast_b_response                = cvm_plusargs::get_bool("axi_sw_fast_write_response") != 0;
            read_latency       = (axi_sw_read_latency_fixed != 0) ? axi_sw_read_latency_fixed : axi_sw_read_latency_max;
            read_latency_fixed = axi_sw_read_latency_fixed != 0;
            /* verilator lint_on BLKSEQ */
            if (read_latency     >= (32'(1)) << CW                                ) $error("Error: +axi_sw_read_latency_max/+axi_sw_read_latency_fixed (%0d) overflows counter width (%0d)", read_latency, CW);
            if (read_latency != 0 && read_latency_timeout_threshold > read_latency) $error("Error: +axi_flush_threshold (%0d) > +axi_sw_read_latency_max/+axi_sw_read_latency_fixed (%0d)", read_latency_timeout_threshold, read_latency);
        end
    end

    localparam AR_HISTORY_Q_MAX = 128;
    localparam AW_HISTORY_Q_MAX = 128;

    logic                   ar_history_empty;
    logic [CW         -1:0] ar_history_q;
    logic [$clog2(AR_HISTORY_Q_MAX+1)-1:0] ar_history_size;

    rv_tester_fifo #(
        .D(AR_HISTORY_Q_MAX),
        .T(logic[CW-1:0])
    ) ar_history (
        .clk,
        .reset_n,
        .push(axi_mst_ar_valid && axi_slv_ar_ready),
        .d(CW'(clocks)),
        .pop (axi_slv_r_valid  && axi_mst_r_ready && axi_slv_r_last), // axi_sw_r_wptr != axi_sw_r_wptr_nxt
        .q(ar_history_q),
        .full(ar_history_full),
        .size(ar_history_size),
        .empty(ar_history_empty)
    );

    logic                   aw_history_empty;
    logic [CW         -1:0] aw_history_q;
    logic [$clog2(AW_HISTORY_Q_MAX+1)-1:0] aw_history_size;

    rv_tester_fifo #(
        .D(AW_HISTORY_Q_MAX),
        .T(logic[CW-1:0])
    ) aw_history (
        .clk,
        .reset_n,
        .push(axi_mst_aw_valid && axi_slv_aw_ready && !fast_b_response),
        .d(CW'(clocks)),
        .pop (axi_slv_b_valid  && axi_mst_b_ready && !fast_b_response), // axi_sw_r_wptr != axi_sw_r_wptr_nxt
        .q(aw_history_q),
        .full(aw_history_full),
        .size(aw_history_size),
        .empty(aw_history_empty)
    );

    assign read_latency_requirement_met = !read_latency_fixed || ar_history_empty || (CW'(clocks) - ar_history_q) >= CW'(read_latency);

    import "DPI-C" function byte unsigned axi_sw_flush(int unsigned location, longint unsigned clock, int unsigned rptr);

    logic flushed;
    logic [$bits(axi_sw_r_wptr)-1:0] axi_sw_r_wptr_prev;

    always_ff @(posedge clk) begin
        if (!reset_n) begin
            flushed <= '0;
        end else if (flushed) begin // on zebu it takes some number of clocks for the export called by this import to take affect
            if (axi_sw_r_wptr_prev != axi_sw_r_wptr) begin
                flushed <= '0;
            end
        end else if (!ar_history_empty && read_latency != 0) begin
            if (r_queue_empty) begin
                automatic logic fifo_near_critical    = ($bits(ar_history_size)'(AR_HISTORY_Q_MAX) - ar_history_size) <= $bits(ar_history_size)'(read_latency_fifo_threshold);
                automatic logic timeout_near_critical = CW'(clocks) - ar_history_q >= CW'(read_latency - read_latency_timeout_threshold);
                automatic logic fifo_critical         = ar_history_full;
                automatic logic timeout_critical      = CW'(clocks) - ar_history_q == CW'(read_latency - read_latency_timeout_threshold);
                if (CW'(clocks) == ar_history_q) $error("Error: clocks wrapped around timer");
                if (fifo_near_critical || timeout_near_critical) begin
                    automatic byte unsigned success;
                    success = axi_sw_flush(LOCATION, clocks, 32'(r_queue_rptr));
                    if (success == '0 && (fifo_critical || timeout_critical)) begin
                        $error("Error: couldn't maintain requested axi read latency");
                    end
                    flushed <= success != '0;
                    axi_sw_r_wptr_prev <= axi_sw_r_wptr;
                end
            end
        end
    end

    import "DPI-C" function void axi_sw_reorder_flush(int unsigned location);

    int unsigned reorder_timeout;
    always_ff @(posedge clk) begin
      if (!reset_n || (reorder_window == '0)) begin
        reorder_timeout <= '0;
      end
      else begin
        // FIXME: aw_history will no longer be in order, on zebu would want to
        // prevent timing out while waiting for resp back
        if (reorder_timeout >= reorder_latency_timeout) begin
          axi_sw_reorder_flush(LOCATION);
          reorder_timeout <= '0;
        end
        else if ((axi_mst_b_ready && !axi_slv_b_valid && !aw_history_empty) || (axi_mst_r_ready && !axi_slv_r_valid && !ar_history_empty)) begin
          reorder_timeout <= reorder_timeout + 1;
        end
        else begin
          reorder_timeout <= '0;
        end
      end
    end

endmodule

module axi_sw_mst #(

    parameter int unsigned ADDR_WIDTH = 32'd0,
    parameter int unsigned DATA_WIDTH = 32'd0,
    parameter int unsigned STRB_WIDTH = DATA_WIDTH / 8,
    parameter int unsigned ID_WIDTH   = 32'd0,
    parameter int unsigned USER_WIDTH = 32'd0,

    parameter int unsigned LOCATION   =     0,
    parameter int NUM                 =    -1,
    parameter string tag = "notag",
    parameter int unsigned AR_Q_MAX   = 32'd0,
    parameter int unsigned AW_Q_MAX   = 32'd0,
    parameter int unsigned W_Q_MAX    = 32'd0,

    parameter type addr_t   = logic [ADDR_WIDTH-1:0],
    parameter type data_t   = logic [DATA_WIDTH-1:0],
    parameter type id_t     = logic [ID_WIDTH  -1:0],
    parameter type strb_t   = logic [STRB_WIDTH-1:0],
    parameter type user_t   = logic [USER_WIDTH-1:0],

    parameter type burst_t  = logic [1:0],
    parameter type resp_t   = logic [1:0],
    parameter type len_t    = logic [7:0],
    parameter type size_t   = logic [2:0],
    parameter type cache_t  = logic [3:0],
    parameter type prot_t   = logic [2:0],
    parameter type qos_t    = logic [3:0],
    parameter type region_t = logic [3:0],
    parameter type atop_t   = logic [5:0],
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
    output cache_t           axi_mst_ar_cache,
    output prot_t            axi_mst_ar_prot,
    output qos_t             axi_mst_ar_qos,
    output region_t          axi_mst_ar_region,
    output user_t            axi_mst_ar_user,

    output logic             axi_mst_aw_valid,
    output id_t              axi_mst_aw_id,
    output addr_t            axi_mst_aw_addr,
    output len_t             axi_mst_aw_len,
    output size_t            axi_mst_aw_size,
    output burst_t           axi_mst_aw_burst,
    output logic             axi_mst_aw_lock,
    output cache_t           axi_mst_aw_cache,
    output prot_t            axi_mst_aw_prot,
    output qos_t             axi_mst_aw_qos,
    output region_t          axi_mst_aw_region,
    output atop_t            axi_mst_aw_atop,
    output user_t            axi_mst_aw_user,

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
        cache_t           cache;
        prot_t            prot;
        qos_t             qos;
        region_t          region;
        user_t            user;
    } ar_t;

    typedef struct packed {
        id_t              id;
        addr_t            addr;
        len_t             len;
        size_t            size;
        burst_t           burst;
        logic             lock;
        cache_t           cache;
        prot_t            prot;
        qos_t             qos;
        region_t          region;
        atop_t            atop;
        user_t            user;
    } aw_t;

    typedef struct packed {
        data_t            data;
        strb_t            strb;
        logic             last;
    } w_t;


    import "DPI-C" context function void axi_sw_mst_reset_ptrs(int unsigned location);

    always @(posedge clk) begin
        if (sys_reset) begin
            /* verilator lint_off BLKSEQ */
            if (LOCATION != cvm_topology::nil) begin
                // FIFO wptr_nxt reset is handled inside AXI_SW_DPI_FIFO macro on
                // sys_reset; ZeBu/ISC disallows DUT calls to TB_EXPORT (so the
                // exported axi_sw_mst_*_reset() functions cannot be invoked here).
                axi_sw_mst_reset_ptrs(LOCATION);
                cvm_registry::set_scope(LOCATION);
            end
            /* verilator lint_on BLKSEQ */
        end
    end

    logic [64-1:0] clocks;
    always_ff @(posedge clk) begin
        if (!reset_n) begin
            clocks <= '0;
        end else begin
            clocks <= clocks + 64'(1);
        end
    end

    logic ar_queue_rptr_incremented, ar_queue_empty;
    logic [$clog2(AR_Q_MAX+1)-1:0] ar_queue_rptr;
    ar_t ar;

    `AXI_SW_DPI_FIFO(axi_sw_mst_ar, ar_t, AR_Q_MAX, clk, sys_reset, reset_n, axi_slv_ar_ready && axi_mst_ar_valid, ar_queue_rptr_incremented, ar_queue_empty, ar, ar_queue_rptr)

    function void axi_sw_mst_ar (int unsigned id, longint unsigned addr, byte unsigned len, byte unsigned size, byte unsigned burst, byte unsigned lock, byte unsigned cache, byte unsigned prot, byte unsigned qos, byte unsigned region, byte unsigned user);
        ar_t p;
        p = '{id: id_t'(id), addr: addr_t'(addr), len: len_t'(len), size: size_t'(size), burst: burst_t'(burst), lock: (1)'(lock), cache: cache_t'(cache), prot: prot_t'(prot), qos: qos_t'(qos), region: region_t'(region), user: user_t'(user)};
        `AXI_SW_DPI_FIFO_PUSH(axi_sw_mst_ar,AR_Q_MAX,p,ar_queue_rptr)
    endfunction
    export "DPI-C" function axi_sw_mst_ar;

    always_comb begin
        axi_mst_ar_valid = reset_n ? !ar_queue_empty : '0;

        axi_mst_ar_id    = 'X;
        axi_mst_ar_addr  = 'X;
        axi_mst_ar_len   = 'X;
        axi_mst_ar_size  = 'X;
        axi_mst_ar_burst = 'X;
        axi_mst_ar_lock  = 'X;
        axi_mst_ar_cache = 'X;
        axi_mst_ar_prot  = 'X;
        axi_mst_ar_qos   = 'X;
        axi_mst_ar_region = 'X;
        axi_mst_ar_user  = 'X;

        if (axi_mst_ar_valid) begin
          axi_mst_ar_id    = ar.id;
          axi_mst_ar_addr  = ar.addr;
          axi_mst_ar_len   = ar.len;
          axi_mst_ar_size  = ar.size;
          axi_mst_ar_burst = ar.burst;
          axi_mst_ar_lock  = ar.lock;
          axi_mst_ar_cache = ar.cache;
          axi_mst_ar_prot  = ar.prot;
          axi_mst_ar_qos   = ar.qos;
          axi_mst_ar_region = ar.region;
          axi_mst_ar_user  = ar.user;
        end
    end

    logic aw_queue_rptr_incremented, aw_queue_empty;
    logic [$clog2(AW_Q_MAX+1)-1:0] aw_queue_rptr;
    aw_t aw;

    `AXI_SW_DPI_FIFO(axi_sw_mst_aw, aw_t, AW_Q_MAX, clk, sys_reset, reset_n, axi_slv_aw_ready && axi_mst_aw_valid, aw_queue_rptr_incremented, aw_queue_empty, aw, aw_queue_rptr)

    function void axi_sw_mst_aw (int unsigned id, longint unsigned addr, byte unsigned len, byte unsigned size, byte unsigned burst,  byte unsigned lock, byte unsigned cache, byte unsigned prot, byte unsigned qos, byte unsigned region, byte unsigned atop, byte unsigned user);
        aw_t p;
        p = '{id: id_t'(id), addr: addr_t'(addr), len: len_t'(len), size: size_t'(size), burst: burst_t'(burst), lock: 1'(lock), cache: cache_t'(cache), prot: prot_t'(prot), qos: qos_t'(qos), region: region_t'(region), atop: atop_t'(atop), user: user_t'(user)};
        `AXI_SW_DPI_FIFO_PUSH(axi_sw_mst_aw,AW_Q_MAX,p,aw_queue_rptr)
    endfunction
    export "DPI-C" function axi_sw_mst_aw;

    always_comb begin
        axi_mst_aw_valid = reset_n ? !aw_queue_empty : '0;

        axi_mst_aw_id    = 'X;
        axi_mst_aw_addr  = 'X;
        axi_mst_aw_len   = 'X;
        axi_mst_aw_size  = 'X;
        axi_mst_aw_burst = 'X;
        axi_mst_aw_lock  = 'X;
        axi_mst_aw_cache = 'X;
        axi_mst_aw_prot  = 'X;
        axi_mst_aw_qos   = 'X;
        axi_mst_aw_region= 'X;
        axi_mst_aw_atop  = 'X;
        axi_mst_aw_user  = 'X;

        if (axi_mst_aw_valid) begin
          axi_mst_aw_id    = aw.id;
          axi_mst_aw_addr  = aw.addr;
          axi_mst_aw_len   = aw.len;
          axi_mst_aw_size  = aw.size;
          axi_mst_aw_burst = aw.burst;
          axi_mst_aw_lock  = aw.lock;
          axi_mst_aw_cache = aw.cache;
          axi_mst_aw_prot  = aw.prot;
          axi_mst_aw_qos   = aw.qos;
          axi_mst_aw_region= aw.region;
          axi_mst_aw_atop  = aw.atop;
          axi_mst_aw_user  = aw.user;
        end
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

    `undef AXI_SW_MST_W

    always_comb begin
        axi_mst_w_valid = reset_n ? !w_queue_empty : '0;

        axi_mst_w_data  = 'X;
        axi_mst_w_strb  = 'X;
        axi_mst_w_last  = 'X;

        if (axi_mst_w_valid) begin
          axi_mst_w_data  = w.data;
          axi_mst_w_strb  = w.strb;
          axi_mst_w_last  = w.last;
        end
    end

    //assign axi_mst_b_ready = '1;
    //assign axi_mst_r_ready = '1;

    int unsigned brdy_hi ;
    int unsigned brdy_low;
    int unsigned rrdy_hi ;
    int unsigned rrdy_low;
    longint unsigned  axi_sw_rsp_toggle_start;
    bit axi_sw_rsp_toggle_en;
    //bit [63:0] clocks;
    always @(posedge clk) begin
        if (sys_reset) begin
            /* verilator lint_off BLKSEQ */
             brdy_hi      = cvm_plusargs::get_int("axi_mst_brdy_high");
             brdy_low     = cvm_plusargs::get_int("axi_mst_brdy_low");
             rrdy_hi      = cvm_plusargs::get_int("axi_mst_rrdy_high");
             rrdy_low     = cvm_plusargs::get_int("axi_mst_rrdy_low");
             /* verilator lint_off WIDTHTRUNC */
             axi_sw_rsp_toggle_en = cvm_plusargs::get_bool("axi_sw_rsp_toggle_en");
             /* verilator lint_on WIDTHTRUNC */
             axi_sw_rsp_toggle_start = cvm_plusargs::get_ulongint("axi_sw_rsp_toggle_start");
             //clocks = 0;
            /* verilator lint_on BLKSEQ */
        end
        else begin
           //clocks = clocks + 1;
        end
    end

    // Internal counters
    logic [31:0] brdy_counter;
    logic brdy_state; // 0 for low, 1 for high

    // Sequential logic for counter and state
    always_ff @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            brdy_counter <= 0;
            brdy_state   <= 0;
        end else begin
            if (brdy_state) begin // High state
                if (brdy_counter < brdy_hi) begin
                    brdy_counter <= brdy_counter + 1;
                end else begin
                    brdy_counter <= 0;
                    brdy_state   <= 0; // Switch to low state
                end
            end else begin // Low state
                if (brdy_counter < brdy_low) begin
                    brdy_counter <= brdy_counter + 1;
                end else begin
                    brdy_counter <= 0;
                    brdy_state   <= 1; // Switch to high state
                end
            end
        end
    end

    // Assign output based on state
    //assign axi_mst_b_ready = axi_sw_rsp_toggle_en ? (axi_sw_rsp_toggle_start < clocks )? brdy_state: 1 : 1;
    assign axi_mst_b_ready = axi_sw_rsp_toggle_en ?
                         ((axi_sw_rsp_toggle_start < clocks) ? brdy_state : 1'b1) :
                         1'b1;
     // Internal counters
    logic [31:0] rrdy_counter;
    logic rrdy_state; // 0 for low, 1 for high

    // Sequential logic for counter and state
    always_ff @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            rrdy_counter <= 0;
            rrdy_state   <= 0;
        end else begin
            if (rrdy_state) begin // High state
                if (rrdy_counter < rrdy_hi) begin
                    rrdy_counter <= rrdy_counter + 1;
                end else begin
                    rrdy_counter <= 0;
                    rrdy_state   <= 0; // Switch to low state
                end
            end else begin // Low state
                if (rrdy_counter < rrdy_low) begin
                    rrdy_counter <= rrdy_counter + 1;
                end else begin
                    rrdy_counter <= 0;
                    rrdy_state   <= 1; // Switch to high state
                end
            end
        end
    end

    // Assign output based on state
    //assign axi_mst_r_ready = rrdy_state;
    assign axi_mst_r_ready = axi_sw_rsp_toggle_en ?
                         ((axi_sw_rsp_toggle_start < clocks) ? rrdy_state : 1'b1) : 1'b1;

    always @(posedge clk) begin
        ar_q_ptrs[0].valid  <= '0;
        aw_q_ptrs[0].valid  <= '0;
        w_q_ptrs[0].valid   <= '0;
        bs[0].valid         <= '0;
        rs[0].valid         <= '0;

        if (reset_n) begin
            ar_q_ptrs[0].valid          <= ar_queue_rptr_incremented;
            ar_q_ptrs[0].data.location  <= LOCATION;
            ar_q_ptrs[0].data.ar_ptr    <= ar_queue_rptr;

            aw_q_ptrs[0].valid          <= aw_queue_rptr_incremented;
            aw_q_ptrs[0].data.location  <= LOCATION;
            aw_q_ptrs[0].data.aw_ptr    <= aw_queue_rptr;

            w_q_ptrs[0].valid         <= w_queue_rptr_incremented;
            w_q_ptrs[0].data.location <= LOCATION;
            w_q_ptrs[0].data.w_ptr    <= w_queue_rptr;

            bs[0].valid         <= axi_slv_b_valid && axi_mst_b_ready;
            bs[0].data.location <= LOCATION;
            bs[0].data.id       <= axi_slv_b_id;
            bs[0].data.resp     <= axi_slv_b_resp;

            rs[0].valid         <= axi_slv_r_valid && axi_mst_r_ready;
            rs[0].data.location <= LOCATION;
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
