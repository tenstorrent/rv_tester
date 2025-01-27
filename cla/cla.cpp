#include "cla.hpp"

REGISTRY_register(cla, CLA, cvm::registry::all);

extern "C" {

  void cla_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
