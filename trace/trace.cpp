#include "trace.hpp"

REGISTRY_register(trace, TRACE, cvm::registry::all);

extern "C" {

  void trace_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
