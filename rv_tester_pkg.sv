package rv_tester_pkg;

    typedef struct packed {
        // FIXME: need per hart interrupt
        logic lcofi;
        logic sgei;
        logic mei;
        logic vsei;
        logic sei;
        logic mti;
        logic vsti;
        logic sti;
        logic msi;
        logic ssi;
    } interrupt_t;

    typedef enum logic [1:0] {
        DTM_NOP   = 2'h0,
        DTM_READ  = 2'h1,
        DTM_WRITE = 2'h2
    } dtm_op_e;

    typedef struct packed {
        logic        dm_wvalid;
        logic [63:0] dm_wdata;
    } dm_write_t;

    typedef struct packed {
        logic [1023:0] pins;
    } aplic_interrupt_t;

    typedef struct packed {
        logic [6:0]  addr;
        dtm_op_e     op;
        logic [31:0] data;
    } dmi_req_t;

    typedef struct packed {
        logic tms;
        logic tdi;
    } jtag_if_t;

    typedef struct packed {
        logic tdo;
        logic tdo_en;
    } jtag_if_out;

    typedef struct packed {
        logic tck;
        logic trst;
    } jtag_if_tck;

    typedef struct packed  {
        logic [31:0] data;
        logic [1:0]  resp;
    } dmi_resp_t;

    typedef struct packed {
        logic terminate;
    } terminate_t;

    typedef longint unsigned c_handle;
    parameter c_handle nil = '0;

endpackage
