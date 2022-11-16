module tb(
    `ifdef VERILATOR
        input vclk
    `endif
);

    // verilator lint_off UNUSED
    // verilator lint_off UNDRIVEN

    localparam ADDR_WIDTH = 64;
    localparam DATA_WIDTH = 512;
    localparam ID_WIDTH   = 8;
    localparam int unsigned STRB_WIDTH = DATA_WIDTH / 8;

    typedef logic [ADDR_WIDTH-1:0] addr_t;
    typedef logic [DATA_WIDTH-1:0] data_t;
    typedef logic [ID_WIDTH  -1:0] id_t  ;
    typedef logic [STRB_WIDTH-1:0] strb_t;

    typedef logic [1:0] burst_t ;
    typedef logic [1:0] resp_t  ;
    typedef logic [7:0] len_t   ;
    typedef logic [2:0] size_t  ;

    logic             axi_mst_ar_valid;
    id_t              axi_mst_ar_id;
    addr_t            axi_mst_ar_addr;
    len_t             axi_mst_ar_len;
    size_t            axi_mst_ar_size;
    burst_t           axi_mst_ar_burst;
    logic             axi_mst_ar_lock;

    logic             axi_mst_aw_valid;
    id_t              axi_mst_aw_id;
    addr_t            axi_mst_aw_addr;
    len_t             axi_mst_aw_len;
    size_t            axi_mst_aw_size;
    burst_t           axi_mst_aw_burst;
    logic             axi_mst_aw_lock;

    logic             axi_mst_w_valid;
    data_t            axi_mst_w_data;
    strb_t            axi_mst_w_strb;
    logic             axi_mst_w_last;

    logic             axi_mst_b_ready;
    logic             axi_mst_r_ready;

    logic             axi_slv_b_valid;
    id_t              axi_slv_b_id;
    resp_t            axi_slv_b_resp;

    logic             axi_slv_r_valid;
    id_t              axi_slv_r_id;
    data_t            axi_slv_r_data;
    resp_t            axi_slv_r_resp;
    logic             axi_slv_r_last;

    logic             axi_slv_aw_ready;
    logic             axi_slv_ar_ready;
    logic             axi_slv_w_ready;

    logic clk, reset_n;

`ifdef VERILATOR
    assign clk = vclk;
`endif
`ifdef VCS
    initial begin
        clk = '0;
        forever #5 clk = ~clk;
    end
`endif

    axi_sw #(
        .ADDR_WIDTH(ADDR_WIDTH),
        .DATA_WIDTH(DATA_WIDTH),
        .ID_WIDTH(ID_WIDTH),
        .POLL_DEFAULT(
            `ifdef VCS
                1
            `else
                0
            `endif
        )
    ) mem (
        .*
    );

    import "DPI-C" function void get_stim(
        input  int unsigned clock,
        output byte finish,
        output byte vreset_n,
        output byte vaw_valid,
        output longint unsigned vaw_addr,
        output byte vaw_len,
        output byte vaw_size,
        output byte vaw_burst,
        output byte var_valid,
        output longint unsigned var_addr,
        output byte var_len,
        output byte var_size,
        output byte var_burst,
        output byte vw_valid,
        output byte vw_data[64],
        output byte vw_strb[8],
        output byte vw_last,
        input  byte r_valid,
        input  byte r_id,
        input  byte r_data[64],
        input  byte r_last
    );

    int unsigned clock = 0;
    always @(posedge clk) begin
        automatic byte vreset_n;
        automatic byte finish;
        automatic byte vaw_valid;
        automatic longint unsigned vaw_addr;
        automatic byte vaw_len;
        automatic byte vaw_size;
        automatic byte vaw_burst;
        automatic byte var_valid;
        automatic longint unsigned var_addr;
        automatic byte var_len;
        automatic byte var_size;
        automatic byte var_burst;
        automatic byte vw_valid;
        automatic byte vw_data[64];
        automatic byte vw_strb[8];
        automatic byte vw_last;
        automatic byte vr_data[64];
        clock <= clock + 1;
        if (axi_slv_r_valid) begin
            for (int i = 0; i < $size(axi_slv_r_data); i++) begin
                vr_data[i] = axi_slv_r_data[8*i +: 8];
            end
        end
        get_stim(
            clock,
            finish,
            vreset_n,
            vaw_valid,
            vaw_addr,
            vaw_len,
            vaw_size,
            vaw_burst,
            var_valid,
            var_addr,
            var_len,
            var_size,
            var_burst,
            vw_valid,
            vw_data,
            vw_strb,
            vw_last,
            axi_slv_r_valid,
            axi_slv_r_id,
            vr_data,
            axi_slv_r_last
        );
        reset_n <= vreset_n;
        axi_mst_aw_valid <= vaw_valid;
        axi_mst_aw_addr  <= vaw_addr ;
        axi_mst_aw_len   <= vaw_len  ;
        axi_mst_aw_size  <= vaw_size ;
        axi_mst_aw_burst <= vaw_burst;
        axi_mst_ar_valid <= var_valid;
        axi_mst_ar_addr  <= var_addr ;
        axi_mst_ar_len   <= var_len  ;
        axi_mst_ar_size  <= var_size ;
        axi_mst_ar_burst <= var_burst;
        axi_mst_w_valid  <= vw_valid ;
        axi_mst_w_last   <= vw_last  ;
        for (int i = 0; i < $size(axi_mst_w_data); i++) begin
            axi_mst_w_data[8*i +: 8] <= vw_data[i];
            axi_mst_w_strb[i]        <= vw_strb[i/8][i%8];
        end
        axi_mst_b_ready  <= 1'b1;
        axi_mst_r_ready  <= 1'b1;
        if (finish) begin
            $finish;
        end
    end

    // verilator lint_on UNUSED
    // verilator lint_on UNDRIVEN

endmodule
