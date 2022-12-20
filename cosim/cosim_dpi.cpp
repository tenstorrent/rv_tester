#include "rvfi.h"

extern "C" {

  rvfi* rvfi_get(int num) {
      static std::unordered_map<int, rvfi> rvfis;
      auto it = rvfis.find(num);

      if (it == rvfis.end()) {
          it = rvfis.emplace(
                  std::piecewise_construct,
                  std::make_tuple(num),
                  std::make_tuple()
                  ).first;
      }
      return &(it->second);
  }

  void rvfi_reset(rvfi* r) {
      r->reset();
  }
}
