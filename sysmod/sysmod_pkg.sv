package sysmod_pkg;

    import "DPI-C" context function chandle sysmod_get(int num);
    function chandle get(int num);
        return sysmod_get(num);
    endfunction

endpackage
