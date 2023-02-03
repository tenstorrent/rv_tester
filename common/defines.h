#pragma once

namespace rv_tester {

  typedef bool terminate_t;

  struct swint_t {
    unsigned hart;
    unsigned val;
  };

  struct timerint_t {
    unsigned hart;
    unsigned val;
  };


}
