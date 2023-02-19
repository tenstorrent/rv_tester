#include "dut_if/rvfi/rvfi.h"

extern "C" {

  void rvfi_reset(cvm::topology::loc_t loc) {
      cvm::messenger<rvfi::reset_t>::signal(
          loc,
          rvfi::reset_t{});
  }
}
