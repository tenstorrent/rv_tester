#include "pwrmgmt.hpp"

REGISTRY_register(pwrmgmt, PWRMGMT, cvm::registry::all);

extern "C" {

  void pwrmgmt_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

  void pwrmgmt_set_reset_count(cvm::topology::loc_t loc, int count) {
    cvm::registry::messenger.signal<int>(
        loc,
        count);
  }

}
