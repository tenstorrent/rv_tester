#include "triggers.hpp"
#include "cvm/random.hpp"

REGISTRY_register(triggers, TRIGGERS, cvm::registry::all);

extern "C" {

  unsigned int get_random_in_range(unsigned int min, unsigned int max) {
    cvm::rand::uniform_dist<uint32_t> rng(min, max);
    return rng();
  }
}
