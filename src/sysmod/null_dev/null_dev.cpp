#include <iostream>
#include "src/sysmod/null_dev/null_dev.h"

void null_dev::write(const transactor::write_t&) {
  return;
}

void null_dev::read(const transactor::read_t&, data_t&) {
  return;
}
