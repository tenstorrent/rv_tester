#include "jtag_driver.hpp"

REGISTRY_register(jtag_driver, JTAG_DRIVER, cvm::registry::all);

extern "C" {

  void jtag_driver_set_scope(cvm::topology::loc_t loc) {
    cvm::log(cvm::FULL, "[jtag_driver_set_scope] loc = {} \n", loc);
    svScope scope = svGetScope();
    cvm::log(cvm::FULL, "[jtag_driver_set_scope] loc1 = {}, scope1 = {} \n", loc, scope);
    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
    cvm::log(cvm::FULL, "[jtag_driver_set_scope] loc2 = {}, scope2 = {} \n", loc, scope);
  }

}
