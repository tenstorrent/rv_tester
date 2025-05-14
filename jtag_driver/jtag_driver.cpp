#include "jtag_driver.hpp"

REGISTRY_register(jtag_driver, JTAG_DRIVER, cvm::registry::all);

extern "C" {

  void jtag_driver_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
