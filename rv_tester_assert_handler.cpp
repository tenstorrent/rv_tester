#include "cvm/logger.hpp"

extern "C" {
  void rv_tester_cvm_terminate(char* msg) {
    cvm::log(cvm::ERROR, std::string(msg));
  }
}
