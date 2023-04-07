#include <cstdint>
#include "cosim/dut_if/rvfi/rvfi_extern.h"
#include "cosim/whisper_cov/arch_sample_extern.h"
#include "svdpi.h"
#include "sysmod/sysmod_extern.h"
#include "transactors/axi_sw/axi_extern.h"
#include "vpi_user.h"

extern "C" {
    // Mocking functions defined by sysmod_extern.h
    void sysmod_timer_interrupt(unsigned hartid, unsigned val) {}
    void sysmod_sw_interrupt(unsigned hartid, unsigned val) {}
    void sysmod_tbox_interrupt(unsigned hartid, unsigned val, unsigned int_val) {}
    void sysmod_dmi_write(unsigned hartid, unsigned upper_val, unsigned lower_val) {}
    void sysmod_terminate(uint8_t call_finish) {}
    // sysmod_extern.h end

    // Mocking functions defined by svdpi.h
    svScope svGetScope() {
        return nullptr;
    }

    svScope svSetScope(const svScope scope) {
        return nullptr;
    }

    svScope svGetScopeFromName(const char* scopeName) {
        return nullptr;
    }

    int svLow(const svOpenArrayHandle h, int d) {
        return 0;
    }

    int svHigh(const svOpenArrayHandle h, int d) {
        return 0;
    }

    void *svGetArrElemPtr(const svOpenArrayHandle, int indx1, ...) {
        return nullptr;
    }

    void *svGetArrElemPtr1(const svOpenArrayHandle, int indx1) {
        return nullptr;
    }
    // svdpi.h end

    // Mocking functions defined by rvfi_extern.h

    void cosim_terminate(uint8_t call_finish) {}

    // rvfi_extern.h end

    // Mocking functions used by axi_sw_extern.h

    void axi_sw_r(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last) {}

    // axi_sw_extern.cpp end

    // Mocking functions defined by arch_sample_extern.h

    void sample_sv(const cp_pkt*) {}

    // arch_sample_extern.h end

    // Mocking functions defined by vpi_user.h

    PLI_INT32 vpi_get_vlog_info(p_vpi_vlog_info vlog_info_p) {
        return 0;
    }

    // vpi_user.h end
}