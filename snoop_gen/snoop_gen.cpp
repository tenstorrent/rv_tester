#include "snoop_gen.hpp"

REGISTRY_register(snoop_gen, SNOOP_GEN, cvm::registry::all);

extern "C" {

  void snoop_gen_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
