#include "interrupts.hpp"

REGISTRY_register(interrupts, INTERRUPTS, cvm::registry::all);

extern "C" {

  void interrupts_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
