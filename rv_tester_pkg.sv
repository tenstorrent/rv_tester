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
    } cfg_t;

    typedef struct packed {
        // FIXME: need per hart interrupt
        logic external; // FIXME need an array for multiple IRQ?
        logic ipi;
        logic timer;
    } interrupt_t;

endpackage
