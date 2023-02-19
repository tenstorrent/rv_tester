package rv_tester_pkg;

    typedef struct packed {
      int unsigned NRET;
      int unsigned ILEN;
      int unsigned XLEN;
      int unsigned VLEN;

      int unsigned AXI_PORTS;

      int unsigned AXI_ADDR_WIDTH;
      int unsigned AXI_DATA_WIDTH;
      int unsigned AXI_ID_WIDTH  ;

      int unsigned PA_WIDTH;
      int unsigned STQ_PORTS;
      int unsigned L1DC_LD_PORTS;
      int unsigned L1DC_ST_PORTS;
      int unsigned L1DC_DATA_WIDTH;
    } cfg_t;

    typedef struct packed {
        // FIXME: need per hart interrupt
        logic mei; // FIXME need an array for multiple IRQ?
        logic sei;
        logic mti;
        logic sti;
        logic msi;
        logic ssi;
    } interrupt_t;

    typedef longint unsigned c_handle; 
    parameter c_handle nil = '0;

endpackage
