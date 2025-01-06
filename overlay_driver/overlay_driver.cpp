#include "overlay_driver.hpp"

REGISTRY_register(overlay_driver, OVERLAY_DRIVER, cvm::registry::all);

extern "C" {

  void overlay_driver_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
