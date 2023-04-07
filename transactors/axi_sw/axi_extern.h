#include "axi.h"

extern "C" {
  void axi_sw_r(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last);
}