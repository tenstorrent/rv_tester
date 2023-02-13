package sysmod_pkg;

    import "DPI-C" function dpic_pkg::c_handle sysmod_get(int num);
    function dpic_pkg::c_handle get(int num);
        return sysmod_get(num);
    endfunction

endpackage
