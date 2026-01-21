#include "triggers.hpp"

REGISTRY_register(triggers, TRIGGERS, cvm::registry::all);

extern "C" {

  void triggers_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
