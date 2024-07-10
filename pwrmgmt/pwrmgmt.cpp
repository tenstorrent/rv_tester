#include "pwrmgmt.hpp"

REGISTRY_register(pwrmgmt, PWRMGMT, cvm::registry::all);

extern "C" {

  void pwrmgmt_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
