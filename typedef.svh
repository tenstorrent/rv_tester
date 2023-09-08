`define AXI_TYPEDEF_AW_CHAN_T_rv(aw_chan_t, axi_id_t, axi_addr_t, axi_len_t, axi_size_t, axi_burst_t, axi_atop_t)  \
  typedef struct packed {                                       \
        axi_id_t                id    ;				\
        axi_addr_t              addr  ;				\
        axi_len_t               len   ;				\
        axi_size_t              size  ;				\
        axi_burst_t             burst ;				\
        logic                   lock  ;				\
        axi_atop_t              atop  ;				\
} aw_chan_t;
`define AXI_TYPEDEF_W_CHAN_T_rv(w_chan_t, axi_data_t, axi_strb_t)  \
  typedef struct packed {						\
        axi_data_t              data   ;				\
        axi_strb_t              strb   ;				\
        logic                   last   ;                                \
  } w_chan_t;
`define AXI_TYPEDEF_B_CHAN_T_rv(b_chan_t, axi_id_t, axi_resp_t)  \
  typedef struct packed { 					\
	axi_id_t                id     ;			\
        axi_resp_t              resp   ; 			\
  } b_chan_t;
`define AXI_TYPEDEF_AR_CHAN_T_rv(ar_chan_t, axi_addr_t, axi_id_t, axi_len_t, axi_size_t, axi_burst_t)  \
  typedef struct packed {                                       \
    axi_id_t              id;                                       \
    axi_addr_t            addr;                                     \
    axi_len_t 		  len;                                      \
    axi_size_t  	  size;                                     \
    axi_burst_t 	  burst;                                    \
    logic            	  lock;                                     \
  } ar_chan_t;
`define AXI_TYPEDEF_R_CHAN_T_rv(r_chan_t, axi_data_t, axi_id_t, axi_resp_t)  \
  typedef struct packed {                                     \
    axi_id_t            id;                                       \
    axi_data_t          data;                                     \
    axi_resp_t 		resp;                                     \
    logic          	last;                                     \
  } r_chan_t;
`define AXI_TYPEDEF_REQ_T_rv(req_t, aw_chan_t, w_chan_t, ar_chan_t)  \
  typedef struct packed {                                         \
    aw_chan_t aw;                                                 \
    logic     aw_valid;                                           \
    w_chan_t  w;                                                  \
    logic     w_valid;                                            \
    logic     b_ready;                                            \
    ar_chan_t ar;                                                 \
    logic     ar_valid;                                           \
    logic     r_ready;                                            \
  } req_t;
`define AXI_TYPEDEF_RESP_T_rv(resp_t, b_chan_t, r_chan_t)  \
  typedef struct packed {                               \
    logic     aw_ready;                                 \
    logic     ar_ready;                                 \
    logic     w_ready;                                  \
    logic     b_valid;                                  \
    b_chan_t  b;                                        \
    logic     r_valid;                                  \
    r_chan_t  r;                                        \
  } resp_t;
////////////////////////////////////////////////////////////////////////////////////////////////////
