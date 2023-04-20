package rv_tester_pkg;

    typedef struct packed {
        // FIXME: need per hart interrupt
        logic mei;
        logic sei;
        logic mti;
        logic sti;
        logic msi;
        logic ssi;
    } interrupt_t;

    typedef struct packed {
        logic        dm_wvalid;
        logic [63:0] dm_wdata;
    } dm_write_t;

    typedef struct packed {
        logic terminate;
    } terminate_t;

    typedef longint unsigned c_handle;
    parameter c_handle nil = '0;

endpackage
